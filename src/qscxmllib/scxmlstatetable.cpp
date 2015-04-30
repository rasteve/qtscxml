/****************************************************************************
 **
 ** Copyright (c) 2015 Digia Plc
 ** For any questions to Digia, please use contact form at http://qt.digia.com/
 **
 ** All Rights Reserved.
 **
 ** NOTICE: All information contained herein is, and remains
 ** the property of Digia Plc and its suppliers,
 ** if any. The intellectual and technical concepts contained
 ** herein are proprietary to Digia Plc
 ** and its suppliers and may be covered by Finnish and Foreign Patents,
 ** patents in process, and are protected by trade secret or copyright law.
 ** Dissemination of this information or reproduction of this material
 ** is strictly forbidden unless prior written permission is obtained
 ** from Digia Plc.
 ****************************************************************************/

#include "scxmlstatetable_p.h"

#include <QAbstractState>
#include <QAbstractTransition>
#include <QState>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QLoggingCategory>
#include <QJSEngine>
#include <QJsonDocument>
#include <QtCore/private/qstatemachine_p.h>

namespace Scxml {
Q_LOGGING_CATEGORY(scxmlLog, "scxml.table")

QEvent::Type ScxmlEvent::scxmlEventType = (QEvent::Type)QEvent::registerEventType();

namespace {
QByteArray objectId(QObject *obj, bool strict = false)
{
    Q_UNUSED(obj);
    Q_UNUSED(strict);
    Q_UNIMPLEMENTED();
    return QByteArray();
}

int parseTime(const QString &t, bool *ok = 0)
{
    if (t.isEmpty()) {
        if (ok)
            *ok = false;
        return -1;
    }
    bool negative = false;
    int startPos = 0;
    if (t[0] == QLatin1Char('-')) {
        negative = true;
        ++startPos;
    } else if (t[0] == QLatin1Char('+')) {
        ++startPos;
    }
    int pos = startPos;
    for (int endPos = t.length(); pos < endPos; ++pos) {
        auto c = t[pos];
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            break;
    }
    if (pos == startPos) {
        if (ok) *ok = false;
        return -1;
    }
    int value = t.midRef(startPos, pos - startPos).toInt(ok);
    if (ok && !*ok) return -1;
    if (t.length() == pos + 1 && t[pos] == QLatin1Char('s')) {
        value *= 1000;
    } else if (t.length() != pos + 2 || t[pos] != QLatin1Char('m') || t[pos + 1] != QLatin1Char('s')) {
        if (ok) *ok = false;
        return -1;
    }
    return negative ? -value : value;
}

class EventBuilder
{
    StateTable* table = 0;
    QString instructionLocation;
    QByteArray event;
    DataModel::EvaluatorString eventexpr;
    QString contents;
    DataModel::EvaluatorString contentExpr;
    QVector<ExecutableContent::Param> params;
    ScxmlEvent::EventType eventType = ScxmlEvent::External;
    QByteArray id;
    QString idLocation;
    QString target;
    DataModel::EvaluatorString targetexpr;
    QString type;
    DataModel::EvaluatorString typeexpr;
    QStringList namelist;

    static QAtomicInt idCounter;
    QByteArray generateId() const
    {
        QByteArray id = QByteArray::number(++idCounter);
        id.prepend("id-");
        return id;
    }

    EventBuilder()
    {}

public:
    EventBuilder(StateTable *table, const QString &instructionLocation, const QByteArray &event,
                 const ExecutableContent::DoneData &doneData)
        : table(table)
        , instructionLocation(instructionLocation)
        , event(event)
        , contents(doneData.contents)
        , contentExpr(doneData.expr)
        , params(doneData.params)
    {}

    EventBuilder(StateTable *table, const ExecutableContent::Send &send)
        : table(table)
        , instructionLocation(send.instructionLocation())
        , event(send.event)
        , eventexpr(send.eventexpr)
        , contents(send.content)
        , params(send.params)
        , id(send.id.toUtf8())
        , idLocation(send.idLocation)
        , target(send.target)
        , targetexpr(send.targetexpr)
        , type(send.type)
        , typeexpr(send.typeexpr)
        , namelist(send.namelist)
    {}

    ScxmlEvent *operator()() { return buildEvent(); }

    ScxmlEvent *buildEvent()
    {
        QByteArray eventName = event;
        bool ok = true;
        if (eventexpr) {
            eventName = eventexpr(&ok).toUtf8();
            ok = true; // ignore failure.
        }

        QVariantList dataValues;
        QStringList dataNames;
        if (params.isEmpty() && namelist.isEmpty()) {
            QVariant data;
            if (contentExpr) {
                data = contentExpr(&ok);
            } else {
                data = contents;
            }
            if (ok) // if evaluation of expr failed, which results in no data property being set on the event. See e.g. test528.
                dataValues.append(data);
        } else {
            if (ExecutableContent::Param::evaluate(params, table, dataValues, dataNames)) {
                foreach (const QString &name, namelist) {
                    dataNames << name;
                    dataValues << table->datamodelJSValues().property(name).toVariant();
                }
            } else {
                // If the evaluation of the <param> tags fails, set _event.data to an empty string.
                // See test488.
                dataValues = QVariantList() << QLatin1String("");
                dataNames.clear();
            }
        }

        QByteArray sendid = id;
        if (!idLocation.isEmpty()) {
            sendid = generateId();
            table->datamodelJSValues().setProperty(idLocation, QString::fromUtf8(sendid));
        }

        QString origin = target;
        if (targetexpr) {
            origin = targetexpr(&ok);
            if (!ok)
                return nullptr;
        }
        if (origin.isEmpty()) {
            if (eventType == ScxmlEvent::External) {
                origin = QStringLiteral("#_internal");
            }
        } else if (!table->isLegalTarget(origin)) {
            // [6.2.4] and test194.
            table->submitError(QByteArray("error.execution"),
                               QStringLiteral("Error in %1: %2 is not a legal target")
                               .arg(instructionLocation, origin),
                               sendid);
            return nullptr;
        } else if (!table->isDispatchableTarget(origin)) {
            // [6.2.4] and test521.
            table->submitError(QByteArray("error.communication"),
                               QStringLiteral("Error in %1: cannot dispatch to target '%2'")
                               .arg(instructionLocation, origin),
                               sendid);
            return nullptr;
        }

        QString origintype = type;
        if (origintype.isEmpty()) {
            // [6.2.5] and test198
            origintype = QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor");
        }
        if (typeexpr) {
            origintype = typeexpr(&ok);
            if (!ok)
                return nullptr;
        }
        if (!origintype.isEmpty() && origintype != QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor")) {
            // [6.2.5] and test199
            table->submitError(QByteArray("error.execution"),
                               QStringLiteral("Error in %1: %2 is not a valid type")
                               .arg(instructionLocation, origintype),
                               sendid);
            return nullptr;
        }

        return new ScxmlEvent(eventName, eventType, dataValues, dataNames, sendid, origin, origintype);
    }

    static ScxmlEvent *errorEvent(const QByteArray &name, const QByteArray &sendid)
    {
        EventBuilder event;
        event.event = name;
        event.eventType = ScxmlEvent::Platform; // Errors are platform events. See e.g. test331.
        // _event.data == null, see test528
        event.id = sendid;
        return event();
    }
};

QAtomicInt EventBuilder::idCounter = QAtomicInt(0);

} // anonymous namespace

namespace ExecutableContent {

bool InstructionSequence::execute() const
{
    foreach (Instruction::Ptr instruction, statements) {
        if (instruction) {
            if (!instruction->execute()) {
                return false;
            }
        }
    }

    return true;
}

bool InstructionSequence::bind()
{
    foreach (Instruction::Ptr instruction, statements)
        if (!instruction->bind())
            return false;
    return true;
}

InstructionSequences::InstructionSequences(QAbstractState *parentState, QAbstractTransition *transition)
    : parentState(parentState)
    , transition(transition)
{
}

InstructionSequences::~InstructionSequences()
{
    qDeleteAll(sequences);
}

InstructionSequence *InstructionSequences::newInstructions()
{
    InstructionSequence *s = new InstructionSequence(parentState, transition);
    sequences.append(s);
    return s;
}

bool InstructionSequences::init()
{
    foreach (InstructionSequence *s, sequences) {
        if (!s->init())
            return false;
    }

    return true;
}

void InstructionSequences::execute()
{
    foreach (const InstructionSequence *sequence, sequences)
        sequence->execute();
}

InstructionSequences::const_iterator InstructionSequences::begin() const
{
    return sequences.begin();
}

InstructionSequences::const_iterator InstructionSequences::end() const
{
    return sequences.end();
}

int InstructionSequences::size() const
{
    return sequences.size();
}

bool InstructionSequences::isEmpty() const
{
    return sequences.isEmpty();
}

const InstructionSequence *InstructionSequences::at(int idx) const
{
    return sequences.at(idx);
}

InstructionSequence *InstructionSequences::last() const
{
    return sequences.last();
}

void InstructionSequences::append(InstructionSequence *s)
{
    sequences.append(s);
}

StateTable *Instruction::table() const {
    if (parentState) {
        if (StateTable *table = qobject_cast<StateTable *>(parentState))
            return table;
        else
            return qobject_cast<StateTable *>(parentState->machine());
    }
    if (transition)
        return qobject_cast<StateTable *>(transition->machine());
    qCWarning(scxmlLog) << "Cannot find StateTable of free standing Instruction";
    return 0;
}

QString Instruction::instructionLocation() const {
    if (transition)
        return QStringLiteral("instruction in transition %1 of state %2")
                .arg(transition->objectName(),
                     (transition->sourceState() ? transition->sourceState()->objectName():
                                                  QStringLiteral("*NULL*")));
    else if (parentState)
        return QStringLiteral("instruction in state %1").arg(parentState->objectName());
    else
        return QStringLiteral("free standing instruction");
}

bool Instruction::init() {
    if (table()->dataBinding() == StateTable::EarlyBinding)
        return bind();
    return true;
}

bool JavaScript::execute() const
{
    StateTable *t = table();
    QJSEngine *e = t->engine();
    if (!e) {
        qWarning(scxmlLog) << "Ignoring javascript in " << instructionLocation()
                           << " as no engine is available";
    }
    if (!compiledFunction.isCallable()) {
        compiledFunction = e->evaluate(QStringLiteral("(function() {\n%1\n})").arg(source),
                                       QStringLiteral("<%1>").arg(instructionLocation()), 0);
        if (!compiledFunction.isCallable()) {
            qWarning(scxmlLog) << "Error compiling" << instructionLocation() << ":"
                               << compiledFunction.toString()  << ", ignoring execution";
            return false;
        }
    }
    qCDebug(scxmlLog) << "executing:" << source;
    QJSValue res = compiledFunction.callWithInstance(t->datamodelJSValues());
    if (res.isError()) {
        t->submitError(QByteArray("error.execution"), QStringLiteral("%1 in %2").arg(res.toString(), instructionLocation()),
                       /*sendid =*/ QByteArray());
        return false;
    } else {
        qCDebug(scxmlLog) << "result:" << res.toVariant();
    }

    return true;
}

bool AssignExpression::execute() const
{
    if (!table() || !table()->engine())
        return false;

    auto dataModel = table()->datamodelJSValues();
    if (dataModel.hasProperty(location)) {
        bool ok = true;
        auto value = table()->evalJSValue(expression, [this, &ok]() -> QString {
                                              ok = false;
                                              return QStringLiteral("%1 with expression %2")
                                              .arg(instructionLocation(), expression);
                                          });
        if (ok)
            dataModel.setProperty(location, value);
        return ok;
    } else {
        table()->submitError(QByteArray("error.execution"),
                             QStringLiteral("Error in %1: location '%2' does not exist.")
                             .arg(instructionLocation(), location),
                             /*sendid =*/ QByteArray());
        return false;
    }
}

InstructionVisitor::~InstructionVisitor()
{}

void InstructionVisitor::accept(const Instruction *instruction) {
    switch (instruction->instructionKind()) {
    case Instruction::Raise:
        visitRaise(static_cast<const Raise *>(instruction));
        break;
    case Instruction::Send:
        visitSend(static_cast<const Send *>(instruction));
        break;
    case Instruction::JavaScript:
        visitJavaScript(static_cast<const JavaScript *>(instruction));
        break;
    case Instruction::AssignJson:
        visitAssignJson(static_cast<const AssignJson *>(instruction));
        break;
    case Instruction::AssignExpression:
        visitAssignExpression(static_cast<const AssignExpression *>(instruction));
        break;
    case Instruction::If: {
        auto ifInstruction = static_cast<const If *>(instruction);
        if (!visitIf(ifInstruction))
            break;
        for (int iblock = 0; iblock < ifInstruction->blocks.size(); ++iblock) {
            const InstructionSequence *block = ifInstruction->blocks.at(iblock);
            if (visitSequence(block)) {
                for (int i = 0; i < block->statements.size(); ++i) {
                    accept(block->statements[i].data());
                }
            }
            endVisitSequence(block);
        }
        endVisitIf(ifInstruction);
        break;
    }
    case Instruction::Foreach: {
        auto foreachInstruction = static_cast<const Foreach *>(instruction);
        if (!visitForeach(foreachInstruction))
            break;
        if (!visitSequence(&foreachInstruction->block))
            break;
        for (int i = 0; i < foreachInstruction->block.statements.size(); ++i)
            accept(foreachInstruction->block.statements[i].data());
        endVisitSequence(&foreachInstruction->block);
        endVisitForeach(foreachInstruction);
        break;
    }
    case Instruction::Log:
        visitLog(static_cast<const Log *>(instruction));
        break;
    case Instruction::Cancel:
        visitCancel(static_cast<const Cancel *>(instruction));
        break;
    case Instruction::Invoke: {
        visitInvoke(static_cast<const Invoke *>(instruction));
        auto invokeInstruction = static_cast<const Invoke *>(instruction);
        if (!visitInvoke(invokeInstruction))
            break;
        if (!visitSequence(&invokeInstruction->finalize))
            break;
        for (int i = 0; i < invokeInstruction->finalize.statements.size(); ++i)
            accept(invokeInstruction->finalize.statements[i].data());
        endVisitSequence(&invokeInstruction->finalize);
        endVisitInvoke(invokeInstruction);
        break;
    }
    case Instruction::Sequence: {
        auto sequenceInstruction = static_cast<const InstructionSequence *>(instruction);
        if (!visitSequence(sequenceInstruction))
            break;
        for (int i = 0; i < sequenceInstruction->statements.size(); ++i) {
            accept(sequenceInstruction->statements[i].data());
        }
        endVisitSequence(sequenceInstruction);
        break;
    }
    }
}

bool If::execute() const
{
    StateTable *t = table();
    for (int i = 0; i < conditions.size(); ++i) {
        if (t->evalValueBool(conditions.at(i), [this]() -> QString { return instructionLocation(); })) {
            return blocks.at(i)->execute();
        }
    }
    if (conditions.size() < blocks.size())
        return blocks.at(conditions.size())->execute();

    return true;
}

bool Foreach::execute() const
{
    Q_ASSERT(table() && table()->engine());

    QJSValue jsArray = table()->datamodelJSValues().property(array);
    if (!jsArray.isArray()) {
        table()->submitError("error.execution", QStringLiteral("invalid array '%1' in %2")
                             .arg(array, instructionLocation()), QByteArray());
        return false;
    }

    if (table()->engine()->evaluate(QStringLiteral("(function(){var %1 = 0})()").arg(item)).isError()) {
        table()->submitError("error.execution", QStringLiteral("invalid item '%1' in %2")
                             .arg(item, instructionLocation()), QByteArray());
        return false;
    }

    const int length = jsArray.property(QStringLiteral("length")).toInt();
    const bool hasIndex = !index.isEmpty();

    for (int currentIndex = 0; currentIndex < length; ++currentIndex) {
        QJSValue currentItem = jsArray.property(static_cast<quint32>(currentIndex));
        table()->datamodelJSValues().setProperty(item, currentItem);
        if (hasIndex)
            table()->datamodelJSValues().setProperty(index, currentIndex);
        if (!block.execute())
            return false;
    }

    return true;
}

bool Send::execute() const
{
    Q_ASSERT(table() && table()->engine());
    QString delay = this->delay;
    if (delayexpr) {
        bool ok = false;
        delay = delayexpr(&ok);
        if (!ok)
            return false;
    }

    ScxmlEvent *event = EventBuilder(table(), *this).buildEvent();
    if (!event)
        return false;

    if (delay.isEmpty()) {
        table()->submitEvent(event);
    } else {
        int msecs = parseTime(delay);
        if (msecs >= 0) {
            table()->submitDelayedEvent(msecs, event);
        } else {
            qCDebug(scxmlLog) << "failed to parse delay time" << delay;
            return false;
        }
    }

    return true;
}

bool Raise::execute() const
{
    table()->submitEvent(event, QVariantList(), QStringList(), ScxmlEvent::Internal);
    return true;
}

bool Log::execute() const
{
    bool ok = true;
    QString str = expr(&ok);
    if (ok)
        table()->doLog(label, str);
    return ok;
}

bool Param::evaluate(StateTable *table, QVariantList &dataValues, QStringList &dataNames) const
{
    bool success = true;
    if (!expr.isEmpty()) {
        auto v = table->evalJSValue(expr, [this,&success]() -> QString {
            success = false;
            return QStringLiteral("param with expr %1").arg(expr);
        }).toVariant();
        dataValues.append(v);
        dataNames.append(name);
    } else if (!location.isEmpty()) {
        auto dataModel = table->datamodelJSValues();
        if (dataModel.hasProperty(location)) {
            dataValues.append(dataModel.property(location).toVariant());
            dataNames.append(name);
        } else {
            table->submitError(QByteArray("error.execution"),
                               QStringLiteral("Error in <param>: %1 is not a valid location")
                               .arg(location),
                               /*sendid =*/ QByteArray());
        }
    } else {
        success = false;
    }
    return success;
}

bool Param::evaluate(const QVector<Param> &params, StateTable *table, QVariantList &dataValues, QStringList &dataNames)
{
    for (const Param &p: params) {
        if (!p.evaluate(table, dataValues, dataNames))
            return false;
    }

    return true;
}

bool Cancel::execute() const
{
    QByteArray e = sendid;
    bool ok = true;
    if (sendidexpr)
        e = sendidexpr(&ok).toUtf8();
    if (ok && !e.isEmpty())
        table()->cancelDelayedEvent(e);
    return ok;
}

bool Invoke::execute() const
{
    Q_ASSERT(false);
    return false;
}
} // namespace ExecutableContent

class DataModelPrivate
{
public:
    StateTable *table;
};

DataModel::DataModel(StateTable *table)
    : d(new DataModelPrivate)
{
    Q_ASSERT(table);
    d->table = table;
}

DataModel::~DataModel()
{
    delete d;
}

StateTable *DataModel::table()
{
    return d->table;
}

QAtomicInt StateTable::m_sessionIdCounter = QAtomicInt(0);

StateTable::StateTable(QObject *parent)
    : QStateMachine(*new StateTablePrivate, parent)
    , m_dataModel(nullptr)
    , m_sessionId(m_sessionIdCounter++)
    , m_initialSetup(this, nullptr)
    , m_engine(nullptr)
    , m_dataBinding(EarlyBinding)
    , m_warnIndirectIdClashes(true)
    , m_queuedEvents(nullptr)
{
    connect(this, &QStateMachine::finished, this, &StateTable::onFinished);
}

StateTable::StateTable(StateTablePrivate &dd, QObject *parent)
    : QStateMachine(dd, parent)
    , m_dataModel(nullptr)
    , m_sessionId(m_sessionIdCounter++)
    , m_initialSetup(this, nullptr)
    , m_engine(nullptr)
    , m_dataBinding(EarlyBinding)
    , m_warnIndirectIdClashes(true)
    , m_queuedEvents(nullptr)
{
    connect(this, &QStateMachine::finished, this, &StateTable::onFinished);
}

StateTable::~StateTable()
{
    delete m_queuedEvents;
}

void StateTable::addId(const QByteArray &, QObject *)
{
    // FIXME: remove this.
    Q_UNIMPLEMENTED();
}

DataModel *StateTable::dataModel() const
{
    return m_dataModel;
}

void StateTable::setDataModel(DataModel *dataModel)
{
    m_dataModel = dataModel;
}

QJSValue StateTable::datamodelJSValues() const {
    return m_dataModelJSValues;
    // QQmlEngine::​setObjectOwnership
}

void StateTable::initializeDataFor(QState *s) {
    if (!engine())
        return;
    foreach (const ScxmlData &data, m_data) {
        if (data.context == s) {
            QJSValue v;
            if (!data.expr.isEmpty())
                v = evalJSValue(data.expr, [this, &data]() -> QString {
                    return QStringLiteral("initializeDataFor with data for %1 defined in state %2)")
                            .arg(data.id, QString::fromUtf8(objectId(data.context)));
                });
            m_dataModelJSValues.setProperty(data.id, v);
        }
    }
}

void StateTable::doLog(const QString &label, const QString &msg)
{
    qCDebug(scxmlLog) << label << ":" << msg;
    emit log(label, msg);
}

bool StateTable::evalValueBool(const QString &expr, std::function<QString()> context, bool defaultValue)
{
    QJSEngine *e = engine();
    if (e) {
        QJSValue v = e->evaluate(QStringLiteral("(function(){ return !!(%1); })()").arg(expr),
                                 QStringLiteral("<expr>"), 1);
        if (v.isError()) {
            submitError(QByteArray("error.execution"),
                        QStringLiteral("%1 in %2").arg(v.toString(), context()),
                        /*sendid =*/ QByteArray());
        } else {
            return v.toBool();
        }
    }
    return defaultValue;
}

QJSValue StateTable::evalJSValue(const QString &expr, std::function<QString()> context,
                     QJSValue defaultValue, bool noRaise)
{
    QString getData = QStringLiteral("(function(){ return (\n%1\n); })()").arg(expr);
    QJSValue v = engine()->evaluate(getData, QStringLiteral("<expr>"), 0);
    if (v.isError() && !noRaise) {
        submitError(QByteArray("error.execution"),
                    QStringLiteral("Error in %1: %2\n<expr>:'%3'")
                    .arg(context(), v.toString(), getData),
                    /*sendid =*/ QByteArray());
        v = defaultValue;
    }
    return v;
}

void StateTable::beginSelectTransitions(QEvent *event)
{
    if (event && event->type() != QEvent::None) {
        switch (event->type()) {
        case QEvent::StateMachineSignal: {
            QStateMachine::SignalEvent* e = (QStateMachine::SignalEvent*)event;
            QByteArray signalName = e->sender()->metaObject()->method(e->signalIndex()).methodSignature();
            //signalName.replace(signalName.indexOf('('), 1, QLatin1Char('.'));
            //signalName.chop(1);
            //if (signalName.endsWith(QLatin1Char('.')))
            //    signalName.chop(1);
            ScxmlEvent::EventType eventType = ScxmlEvent::External;
            QObject *s = e->sender();
            if (s == this) {
                if (signalName.startsWith(QByteArray("event_"))){
                    _event.reset(signalName.mid(6), eventType, e->arguments());
                    break;
                } else {
                    qCWarning(scxmlLog) << "Unexpected signal event sent to StateMachine "
                                        << _name << ":" << signalName;
                }
            }
            QByteArray senderName = QByteArray("@0");
            if (s) {
                senderName = objectId(s, true);
                if (senderName.isEmpty() && !s->objectName().isEmpty())
                    senderName = s->objectName().toUtf8();
            }
            QList<QByteArray> namePieces;
            namePieces << QByteArray("qsignal") << senderName << signalName;
            QByteArray eventName = namePieces.join('.');
            _event.reset(eventName, eventType, e->arguments());
        } break;
        case QEvent::StateMachineWrapped: {
            QStateMachine::WrappedEvent * e = (QStateMachine::WrappedEvent *)event;
            QObject *s = e->object();
            QByteArray senderName = QByteArray("@0");
            if (s) {
                senderName = objectId(s, true);
                if (senderName.isEmpty() && !s->objectName().isEmpty())
                    senderName = s->objectName().toUtf8();
            }
            QEvent::Type qeventType = e->event()->type();
            QByteArray eventName;
            QMetaObject metaObject = QEvent::staticMetaObject;
            int maxIenum = metaObject.enumeratorCount();
            for (int ienum = metaObject.enumeratorOffset(); ienum < maxIenum; ++ienum) {
                QMetaEnum en = metaObject.enumerator(ienum);
                if (QByteArray(en.name()) == QByteArray("Type")) {
                    eventName = QByteArray(en.valueToKey(qeventType));
                    break;
                }
            }
            if (eventName.isEmpty())
                eventName = QStringLiteral("E%1").arg((int)qeventType).toUtf8();
            QList<QByteArray> namePieces;
            namePieces << QByteArray("qevent") << senderName << eventName;
            QByteArray name = namePieces.join('.');
            ScxmlEvent::EventType eventType = ScxmlEvent::External;
            // use e->spontaneous(); to choose internal/external?
            _event.reset(name, eventType); // put something more in data for some elements like keyEvents and mouseEvents?
        } break;
        default:
            if (event->type() == ScxmlEvent::scxmlEventType) {
                _event = *static_cast<ScxmlEvent *>(event);
            } else {
                QEvent::Type qeventType = event->type();
                QByteArray eventName = QStringLiteral("qdirectevent.E%1").arg((int)qeventType).toUtf8();
                _event.reset(eventName);
                qCWarning(scxmlLog) << "Unexpected event directly sent to StateMachine "
                                    << _name << ":" << event->type();
            }
            break;
        }
    } else {
        _event.clear();
    }
    assignEvent();
}

void StateTable::beginMicrostep(QEvent *event)
{
    qCDebug(scxmlLog) << _name << " started microstep from state" << currentStates()
                      << "with event" << _event.name() << "from event type" << event->type();
}

void StateTable::endMicrostep(QEvent *event)
{
    Q_UNUSED(event);
    qCDebug(scxmlLog) << _name << " finished microstep in state (" << currentStates() << ")";
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

void StateTablePrivate::noMicrostep()
{
    Q_Q(StateTable);
    qCDebug(scxmlLog) << q->_name << " had no transition, stays in state (" << q->currentStates() << ")";
}

void StateTablePrivate::processedPendingEvents(bool didChange)
{
    Q_Q(StateTable);
    qCDebug(scxmlLog) << q->_name << " finishedPendingEvents " << didChange << " in state ("
                      << q->currentStates() << ")";
    emit q->reachedStableState(didChange);
}

void StateTablePrivate::beginMacrostep()
{
}

void StateTablePrivate::endMacrostep(bool didChange)
{
    Q_Q(StateTable);
    qCDebug(scxmlLog) << q->_name << " endMacrostep " << didChange << " in state ("
                      << q->currentStates() << ")";
}

void StateTablePrivate::emitStateFinished(QState *forState, QFinalState *guiltyState)
{
    Q_Q(StateTable);

    if (ScxmlFinalState *finalState = qobject_cast<ScxmlFinalState *>(guiltyState)) {
        if (!q->isRunning())
            return;
        const ExecutableContent::DoneData &doneData = finalState->doneData;

        QByteArray eventName = forState->objectName().toUtf8();
        eventName.prepend("done.state.");
        EventBuilder event(q, QStringLiteral("<final>"), eventName, doneData);
        qCDebug(scxmlLog) << q->_name << ": submitting event" << eventName;
        q->submitEvent(event());
    }

    QStateMachinePrivate::emitStateFinished(forState, guiltyState);
}

void StateTablePrivate::startupHook()
{
    Q_Q(StateTable);

    q->submitQueuedEvents();
}

#endif // QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

int StateTablePrivate::eventIdForDelayedEvent(const QByteArray &scxmlEventId)
{
    QMutexLocker locker(&delayedEventsMutex);

    QHash<int, DelayedEvent>::const_iterator it;
    for (it = delayedEvents.constBegin(); it != delayedEvents.constEnd(); ++it) {
        if (ScxmlEvent *e = dynamic_cast<ScxmlEvent *>(it->event)) {
            if (e->sendid() == scxmlEventId) {
                return it.key();
            }
        }
    }

    return -1;
}

QStringList StateTable::currentStates(bool compress)
{
    QSet<QAbstractState *> config = d_func()->configuration;
    if (compress)
        foreach (const QAbstractState *s, d_func()->configuration)
            config.remove(s->parentState());
    QStringList res;
    foreach (const QAbstractState *s, config) {
        QString id = s->objectName();
        if (!id.isEmpty()) {
            res.append(id);
        }
    }
    std::sort(res.begin(), res.end());
    return res;
}

void StateTable::assignEvent() {
    if (m_engine)
        m_dataModelJSValues.setProperty(QStringLiteral("_event"), _event.jsValue(m_engine));
}

void StateTable::setupDataModel()
{
    if (!engine())
        return;
    qCDebug(scxmlLog) << "initializing the datamodel";
    setupSystemVariables();
    foreach (const ScxmlData &data ,m_data) {
        QJSValue v(QJSValue::UndefinedValue); // See B.2.1, and test456.
        if ((dataBinding() == EarlyBinding || !data.context || data.context == this)
                && !data.expr.isEmpty())
            v = evalJSValue(data.expr, [this, &data]() -> QString {
                return QStringLiteral("setupDataModel with data for %1 defined in state '%2'")
                        .arg(data.id, QString::fromUtf8(objectId(data.context)));
            });
        qCDebug(scxmlLog) << "setting datamodel property" << data.id << "to" << v.toVariant();
        m_dataModelJSValues.setProperty(data.id, v);
        Q_ASSERT(m_dataModelJSValues.hasProperty(data.id));
    }
}

void StateTable::setupSystemVariables()
{
    m_dataModelJSValues.setProperty(QStringLiteral("_sessionid"),
                                    QStringLiteral("session%1").arg(m_sessionId));

    m_dataModelJSValues.setProperty(QStringLiteral("_name"), _name);

    auto scxml = engine()->newObject();
    scxml.setProperty(QStringLiteral("location"), QStringLiteral("TODO")); // TODO
    auto ioProcs = engine()->newObject();
    ioProcs.setProperty(QStringLiteral("scxml"), scxml);
    m_dataModelJSValues.setProperty(QStringLiteral("_ioprocessors"), ioProcs);

    auto platformVars = PlatformProperties::create(engine(), this);
    m_dataModelJSValues.setProperty(QStringLiteral("_x"), platformVars->jsValue());

    m_dataModelJSValues.setProperty(QStringLiteral("In"),
                                    engine()->evaluate(QStringLiteral("function(id){return _x.In(id);}")));
}

void StateTable::executeInitialSetup()
{
    m_initialSetup.execute();
}

bool loopOnSubStates(QState *startState,
                     std::function<bool(QState *)> enteringState,
                     std::function<void(QState *)> exitingState,
                     std::function<void(QAbstractState *)> inAbstractState)
{
    QList<int> pos;
    QState *parentAtt = startState;
    QObjectList childs = startState->children();
    pos << 0;
    while (!pos.isEmpty()) {
        bool goingDeeper = false;
        for (int i = pos.last(); i < childs.size() ; ++i) {
            if (QAbstractState *as = qobject_cast<QAbstractState *>(childs.at(i))) {
                if (QState *s = qobject_cast<QState *>(as)) {
                    if (enteringState && !enteringState(s))
                        continue;
                    pos.last() = i + 1;
                    parentAtt = s;
                    childs = s->children();
                    pos << 0;
                    goingDeeper = !childs.isEmpty();
                    break;
                } else if (inAbstractState) {
                    inAbstractState(as);
                }
            }
        }
        if (!goingDeeper) {
            do {
                pos.removeLast();
                if (pos.isEmpty())
                    break;
                if (exitingState)
                    exitingState(parentAtt);
                parentAtt = parentAtt->parentState();
                childs = parentAtt->children();
            } while (!pos.isEmpty() && pos.last() >= childs.size());
        }
    }
    return true;
}

bool StateTable::init()
{
    setupDataModel();
    executeInitialSetup();

    bool res = true;
    loopOnSubStates(this, std::function<bool(QState *)>(), [&res](QState *state) {
        if (ScxmlState *s = qobject_cast<ScxmlState *>(state))
            if (!s->init())
                res = false;
        if (ScxmlFinalState *s = qobject_cast<ScxmlFinalState *>(state))
            if (!s->init())
                res = false;
        foreach (QAbstractTransition *t, state->transitions()) {
            if (ScxmlTransition *scTransition = qobject_cast<ScxmlTransition *>(t))
                if (!scTransition->init())
                    res = false;
        }
    });
    foreach (QAbstractTransition *t, transitions()) {
        if (ScxmlTransition *scTransition = qobject_cast<ScxmlTransition *>(t))
            if (!scTransition->init())
                res = false;
    }
    return res;
}

QJSEngine *StateTable::engine() const
{
    return m_engine;
}

void StateTable::setEngine(QJSEngine *engine)
{
    m_engine = engine;
    if (engine)
        m_dataModelJSValues = engine->globalObject();
}

void StateTable::submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid)
{
    qCDebug(scxmlLog) << "machine" << _name << "had error" << type << ":" << msg;
    submitEvent(EventBuilder::errorEvent(type, sendid));
}

void StateTable::submitEvent(ScxmlEvent *e)
{
    if (!e)
        return;

    EventPriority priority = e->eventType() == ScxmlEvent::External ? QStateMachine::NormalPriority
                                                                    : QStateMachine::HighPriority;

    if (isRunning())
        postEvent(e, priority);
    else
        queueEvent(e, priority);
}

void StateTable::submitEvent(const QByteArray &event, const QVariantList &dataValues,
                             const QStringList &dataNames, ScxmlEvent::EventType type,
                             const QByteArray &sendid, const QString &origin,
                             const QString &origintype, const QByteArray &invokeid)
{
    qCDebug(scxmlLog) << _name << ": submitting event" << event;

    ScxmlEvent *e = new ScxmlEvent(event, type, dataValues, dataNames, sendid, origin, origintype, invokeid);
    submitEvent(e);
}

void StateTable::submitDelayedEvent(int delayInMiliSecs, ScxmlEvent *e)
{
    Q_ASSERT(delayInMiliSecs > 0);

    if (!e)
        return;

    qCDebug(scxmlLog) << _name << ": submitting event" << e->name() << "with delay" << delayInMiliSecs << "ms" << "and sendid" << e->sendid();

    Q_ASSERT(e->eventType() == ScxmlEvent::External);
    int id = postDelayedEvent(e, delayInMiliSecs);

    qCDebug(scxmlLog) << _name << ": delayed event" << e->name() << "(" << e << ") got id:" << id;
}

void StateTable::cancelDelayedEvent(const QByteArray &sendid)
{
    Q_D(StateTable);

    int id = d->eventIdForDelayedEvent(sendid);

    qCDebug(scxmlLog) << _name << ": canceling event" << sendid << "with id" << id;

    if (id != -1)
        QStateMachine::cancelDelayedEvent(id);
}

void StateTable::queueEvent(ScxmlEvent *event, EventPriority priority)
{
    qCDebug(scxmlLog) << _name << ": queueing event" << event->name();

    if (!m_queuedEvents)
        m_queuedEvents = new QVector<QueuedEvent>();
    m_queuedEvents->append({event, priority});
}

void StateTable::submitQueuedEvents()
{
    qCDebug(scxmlLog) << _name << ": submitting queued events";

    if (m_queuedEvents) {
        foreach (const QueuedEvent &e, *m_queuedEvents)
            postEvent(e.event, e.priority);
        delete m_queuedEvents;
        m_queuedEvents = 0;
    }
}

bool StateTable::isLegalTarget(const QString &target) const
{
    return target.startsWith(QLatin1Char('#'));
}

bool StateTable::isDispatchableTarget(const QString &target) const
{
    return target == QStringLiteral("#_internal");
}

void StateTable::onFinished()
{
    // The final state is also a stable state.
    emit reachedStableState(true);
}

ScxmlEvent::ScxmlEvent(const QByteArray &name, ScxmlEvent::EventType eventType,
                       const QVariantList &dataValues, const QStringList &dataNames,
                       const QByteArray &sendid, const QString &origin,
                       const QString &origintype, const QByteArray &invokeid)
    : QEvent(scxmlEventType), m_name(name), m_type(eventType), m_dataValues(dataValues), m_dataNames(dataNames)
    , m_sendid(sendid), m_origin(origin), m_origintype(origintype), m_invokeid(invokeid)
{ }

QString ScxmlEvent::scxmlType() const {
    switch (m_type) {
    case Platform:
        return QLatin1String("platform");
    case Internal:
        return QLatin1String("internal");
    case External:
        break;
    }
    return QLatin1String("external");
}

QJSValue ScxmlEvent::data(QJSEngine *engine) const {
    if (dataNames().isEmpty()) {
        if (dataValues().size() == 0) {
            return QJSValue(QJSValue::NullValue);
        } else if (dataValues().size() == 1) {
            QString data = dataValues().first().toString();
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError)
                return engine->toScriptValue(doc.toVariant());
            else
                return engine->toScriptValue(data);
        } else {
            Q_UNREACHABLE();
            return QJSValue(QJSValue::UndefinedValue);
        }
    } else {
        auto data = engine->newObject();

        for (int i = 0, ei = std::min(dataNames().size(), dataValues().size()); i != ei; ++i) {
            data.setProperty(dataNames().at(i), engine->toScriptValue(dataValues().at(i)));
        }

        return data;
    }
}

void ScxmlEvent::reset(const QByteArray &name, ScxmlEvent::EventType eventType, QVariantList dataValues,
                       const QByteArray &sendid, const QString &origin,
                       const QString &origintype, const QByteArray &invokeid) {
    m_name = name;
    m_type = eventType;
    m_sendid = sendid;
    m_origin = origin;
    m_origintype = origintype;
    m_invokeid = invokeid;
    m_dataValues = dataValues;
}

void ScxmlEvent::clear() {
    m_name = QByteArray();
    m_type = External;
    m_sendid = QByteArray();
    m_origin = QString();
    m_origintype = QString();
    m_invokeid = QByteArray();
    m_dataValues = QVariantList();
}

QJSValue ScxmlEvent::jsValue(QJSEngine *engine) const {
    QJSValue res = engine->newObject();
    QJSValue dataValue = data(engine);
    res.setProperty(QStringLiteral("data"), dataValue.isNull() ? QJSValue(QJSValue::UndefinedValue)
                                                               : dataValue);
    res.setProperty(QStringLiteral("invokeid"), invokeid().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                     : engine->toScriptValue(QString::fromUtf8(invokeid())));
    if (!origintype().isEmpty())
        res.setProperty(QStringLiteral("origintype"), engine->toScriptValue(origintype()));
    res.setProperty(QStringLiteral("origin"), origin().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                 : engine->toScriptValue(origin()) );
    res.setProperty(QStringLiteral("sendid"), sendid().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                 : engine->toScriptValue(QString::fromUtf8(sendid())));
    res.setProperty(QStringLiteral("type"), engine->toScriptValue(scxmlType()));
    res.setProperty(QStringLiteral("name"), engine->toScriptValue(QString::fromUtf8(name())));
    res.setProperty(QStringLiteral("raw"), QStringLiteral("unsupported")); // See test178
                                                                           // TODO: document this
    return res;
}

/////////////
ScxmlBaseTransition::ScxmlBaseTransition(QState *sourceState, const QList<QByteArray> &eventSelector) :
    QAbstractTransition(sourceState), eventSelector(eventSelector) { }

ScxmlBaseTransition::ScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                                         const QList<QByteArray> &eventSelector)
    : QAbstractTransition(dd, parent), eventSelector(eventSelector)
{ }

StateTable *ScxmlBaseTransition::table() const {
    if (StateTable *t = qobject_cast<StateTable *>(parent()))
        return t;
    if (sourceState())
        return qobject_cast<StateTable *>(sourceState()->machine());
    qCWarning(scxmlLog) << "could not resolve StateTable in " << transitionLocation();
    return 0;
}

QString ScxmlBaseTransition::transitionLocation() const {
    if (QState *state = sourceState()) {
        QString stateName = state->objectName();
        int transitionIndex = state->transitions().indexOf(const_cast<ScxmlBaseTransition *>(this));
        return QStringLiteral("transition #%1 in state %2").arg(transitionIndex).arg(stateName);
    }
    return QStringLiteral("unbound transition @%1").arg((size_t)(void*)this);
}

bool ScxmlBaseTransition::eventTest(QEvent *event) {
    if (eventSelector.isEmpty())
        return true;
    if (event->type() == QEvent::None)
        return false;
    StateTable *stateTable = table();
    Q_ASSERT(stateTable);
    QByteArray eventName = stateTable->_event.name();
    bool selected = false;
    foreach (QByteArray eventStr, eventSelector) {
        if (eventStr == "*") {
            selected = true;
            break;
        }
        if (eventStr.endsWith(".*"))
            eventStr.chop(2);
        if (eventName.startsWith(eventStr)) {
            char nextC = '.';
            if (eventName.size() > eventStr.size())
                nextC = eventName.at(eventStr.size());
            if (nextC == '.' || nextC == '(') {
                selected = true;
                if (event->type() != QEvent::StateMachineSignal && event->type() != ScxmlEvent::scxmlEventType) {
                    qCWarning(scxmlLog) << "unexpected triggering of event " << eventName
                                        << " with type " << event->type() << " detected in "
                                        << transitionLocation();
                }
                break;
            }
        }
    }
#ifdef SCXML_DEBUG
    if (!m_concreteTransitions.isEmpty() && event->type() == QEvent::StateMachineSignal
            && static_cast<QStateMachine::SignalEvent *>(event)->sender() != stateTable) {
        bool selected2 = false;
        foreach (TransitionPtr t, m_concreteTransitions) {
            if (t->subEventTest(event))
                selected2 = true;
        }
        if (selected != selected2) {
            qCWarning(scxmlLog) << "text based triggering and signal based triggering differs for event"
                                << eventName << " text based comparison with '"
                                << eventSelector.join(' ')
                                << "' gives value " << selected
                                << " while the underlying concrete transitions give "
                                << selected2 << " in " << transitionLocation();
        }
    }
#endif
    return selected;
}

bool ScxmlBaseTransition::clear() {
    foreach (TransitionPtr t, m_concreteTransitions)
        sourceState()->removeTransition(t.data());
    m_concreteTransitions.clear();
    return true;
}

bool ScxmlBaseTransition::init() {
    Q_ASSERT(m_concreteTransitions.isEmpty());
    if (eventSelector.isEmpty())
        return true;
    bool failure = false;
    foreach (const QByteArray &eventStr, eventSelector) {
        QList<QByteArray> selector = eventStr.split('.');
        if (selector.isEmpty())
            continue;
        else if (selector.first() == QByteArray("qsignal")) {
            // FIXME: the sender cannot be found this way anymore. We need some tests before we enable/fix this code.
            if (true) {
                Q_UNIMPLEMENTED();
            } else {
            // FIXME starts here.
//            StateTable *stateTable = table();
            if (selector.count() < 2) {
                qCWarning(scxmlLog) << "qeventSelector requires a sender id in " << transitionLocation();
                failure = true;
                continue;
            }
            QObject *sender = nullptr; // stateTable->idToValue<QObject>(selector.value(1));
            if (!sender) {
                qCWarning(scxmlLog) << "could not find object with id " << selector.value(1)
                                    << " used in " << transitionLocation();
                failure = true;
                continue;
            }
            QByteArray methodName = selector.value(2);
            bool partial = !methodName.contains('(');
            int minMethodLen = methodName.size();
            const QMetaObject *metaObject = sender->metaObject();
            int maxImethod = metaObject->methodCount();
            for (int imethod = 0; imethod < maxImethod; ++imethod){
                QMetaMethod m = metaObject->method(imethod);
                if (m.methodType() != QMetaMethod::Signal) continue;
                QByteArray mName = m.methodSignature();
                if (methodName == mName // exact match
                        || ( // partial match, but excluding deleteLater() destroyed() that must be explicitly included
                             partial && mName.size() > minMethodLen && mName != QByteArray("deleteLater()")
                             && mName != QByteArray("destroyed()")
                             && (methodName.isEmpty() || (mName.startsWith(methodName)
                                                          && mName.at(methodName.size()) == '('))))
                {
                    ConcreteSignalTransition *newT = new ConcreteSignalTransition(sender, mName.data(), sourceState());
                    newT->setTargetState(targetState()); // avoid?
                    m_concreteTransitions << TransitionPtr(newT);
                }
            }
            if (m_concreteTransitions.isEmpty()) {
                QList<QByteArray> knownSignals;
                for (int imethod = 0; imethod < maxImethod; ++imethod){
                    QMetaMethod m = metaObject->method(imethod);
                    if (m.methodType() != QMetaMethod::Signal) continue;
                    QByteArray mName = m.methodSignature();
                    knownSignals.append(mName);
                }
                qCWarning(scxmlLog) << "eventSelector failed to match anything in "
                                    << transitionLocation() << ", selector is: "
                                    << eventSelector.join(' ') << " and known signals are:\n  "
                                    << knownSignals.join(' ');
                failure = true; // ignore instead??
            }
            } // end of FIXME
        } else if (selector.first() == QByteArray("qevent")){
            qCWarning(scxmlLog) << "selector of qevent type to implement";
            failure = true;
        } else {
            // this is expected to be a custom scxml event, no binding required
        }
    }
    return !failure;
}

QList<QByteArray> ScxmlBaseTransition::targetIds() const {
    QList<QByteArray> res;
    foreach (QAbstractState *s, targetStates())
        res << s->objectName().toUtf8();
    return res;
}

void ScxmlBaseTransition::onTransition(QEvent *event)
{
    Q_UNUSED(event);
}

/////////////

static QList<QByteArray> filterEmpty(const QList<QByteArray> &events) {
    QList<QByteArray> res;
    int oldI = 0;
    for (int i = 0; i < events.size(); ++i) {
        if (events.at(i).isEmpty()) {
            res.append(events.mid(oldI, i - oldI));
            oldI = i + 1;
        }
    }
    if (oldI > 0) {
        res.append(events.mid(oldI));
        return res;
    }
    return events;
}

ScxmlTransition::ScxmlTransition(QState *sourceState, const QList<QByteArray> &eventSelector,
                                 const QList<QByteArray> &targetIds, const QString &conditionalExp)
    : ScxmlBaseTransition(sourceState, filterEmpty(eventSelector))
    , conditionalExp(conditionalExp)
    , type(ScxmlEvent::External)
    , instructionsOnTransition(sourceState, this)
    , m_targetIds(filterEmpty(targetIds))
{}

bool ScxmlTransition::eventTest(QEvent *event)
{
    if (ScxmlBaseTransition::eventTest(event)) {
        if (conditionalExp.isEmpty())
            return true;

//        qCDebug(scxmlLog) << qPrintable(table()->engine()->evaluate(QLatin1String("JSON.stringify(_event)")).toString());
        bool ok = true;
        bool result = table()->evalValueBool(conditionalExp, [this,&ok]() -> QString {
                                                 ok = false;
                                                 return transitionLocation();
                                             });
        qCDebug(scxmlLog) << Q_FUNC_INFO << ":" << conditionalExp << "evaluated to" << result;
        return ok && result;
    }

    return false;
}

void ScxmlTransition::onTransition(QEvent *)
{
    instructionsOnTransition.execute();
}

StateTable *ScxmlState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlState::init()
{
    m_dataInitialized = (table()->dataBinding() == StateTable::EarlyBinding);
    if (!onEntryInstructions.init())
        return false;
    if (!onExitInstructions.init())
        return false;
    return true;
}

QString ScxmlState::stateLocation() const
{
    return QStringLiteral("State %1").arg(objectName());
}

void ScxmlState::onEntry(QEvent *event) {
    if (!m_dataInitialized) {
        m_dataInitialized = true;
        // this might actually be a bit too late (parallel states might already have been entered)
        table()->initializeDataFor(this);
    }
    QState::onEntry(event);
    onEntryInstructions.execute();
}

void ScxmlState::onExit(QEvent *event) {
    QState::onExit(event);
    onExitInstructions.execute();
}

StateTable *ScxmlFinalState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlFinalState::init()
{
    if (!onEntryInstructions.init())
        return false;
    if (!onExitInstructions.init())
        return false;
    return true;
}

void ScxmlFinalState::onEntry(QEvent *event) {
    QFinalState::onEntry(event);
    onEntryInstructions.execute();
}

void ScxmlFinalState::onExit(QEvent *event) {
    QFinalState::onExit(event);
    onExitInstructions.execute();
}

bool XmlNode::isText() const {
    return m_name.startsWith(QLatin1Char('$'));
}

QStringList XmlNode::texts() const {
    QStringList res;
    foreach (const XmlNode &node, m_childs)
        if (node.isText())
            res.append(node.text());
    return res;
}

QString XmlNode::text() const {
    if (isText())
        return m_name.mid(1);
    QStringList res;
    foreach (const XmlNode &node, m_childs)
        if (node.isText())
            res << node.text();
    return res.join(QString());
}

void XmlNode::addText(const QString &value) {
    m_childs.append(XmlNode(value));
}

void XmlNode::addTag(const QString &name, const QString &xmlns, const QXmlStreamAttributes &attributes, QVector<XmlNode> childs) {
    static QRegExp spaceRe(QLatin1String("^\\s*$"));
    if (!m_childs.isEmpty() && m_childs.last().isText()
            && spaceRe.exactMatch(m_childs.last().text()))
        m_childs.last() = XmlNode(name, ((m_namespace == xmlns) ? m_namespace : xmlns),
                                  attributes, childs);
    else
        m_childs.last() = XmlNode(name, ((m_namespace == xmlns) ? m_namespace : xmlns),
                                  attributes, childs);
}

QString XmlNode::name() const {
    if (!m_name.startsWith(QLatin1Char('$')))
        return m_name;
    return QString();
}

QXmlStreamAttributes XmlNode::attributes() const {
    return m_attributes;
}

bool XmlNode::loopOnAttributes(std::function<bool (const QXmlStreamAttribute &)> l) {
    foreach (const QXmlStreamAttribute &a, m_attributes) {
        if (!l(a))
            return false;
    }
    return true;
}

bool XmlNode::loopOnText(std::function<bool (const QString &)> l) const {
    foreach (const XmlNode &n, m_childs) {
        if (n.isText() && !l(n.text()))
            return false;
    }
    return true;
}

bool XmlNode::loopOnChilds(std::function<bool (const XmlNode &)> l) const {
    foreach (const XmlNode &n, m_childs) {
        if (!l(n))
            return false;
    }
    return true;
}

bool XmlNode::loopOnTags(std::function<bool (const XmlNode &)> l) const {
    foreach (const XmlNode &n, m_childs) {
        if (!n.isText() && !l(n))
            return false;
    }
    return true;
}

void XmlNode::dump(QXmlStreamWriter &s) const
{
    if (isText()) {
        s.writeCharacters(text());
    } else {
        s.writeStartElement(m_namespace, m_name);
        s.writeAttributes(m_attributes);
        loopOnChilds([this, &s](const XmlNode &node) { node.dump(s); return true; }); // avoid recursion?
        s.writeEndElement();
    }
}

PlatformProperties *PlatformProperties::create(QJSEngine *engine, StateTable *table)
{
    PlatformProperties *pp = new PlatformProperties(engine);
    pp->m_table = table;
    pp->m_jsValue = engine->newQObject(pp);
    return pp;
}

QString PlatformProperties::marks() const
{
    return QStringLiteral("the spot");
}

bool PlatformProperties::In(const QString &stateName)
{
    foreach (QAbstractState *s, table()->configuration()) {
        if (s->objectName() == stateName)
            return true;
    }

    return false;
}

} // namespace Scxml
