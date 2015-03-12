/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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
#include <QLoggingCategory>
#include <QJSEngine>
#include <QtCore/private/qstatemachine_p.h>

namespace Scxml {
Q_LOGGING_CATEGORY(scxmlLog, "scxml.table")

QEvent::Type ScxmlEvent::scxmlEventType = (QEvent::Type)QEvent::registerEventType();

namespace ExecutableContent {

void InstructionSequence::execute()
{
    foreach (Instruction::Ptr instruction, statements)
        if (instruction)
            instruction->execute();
}

bool InstructionSequence::bind()
{
    foreach (Instruction::Ptr instruction, statements)
        if (!instruction->bind())
            return false;
    return true;
}

StateTable *Instruction::table() const {
    if (parentState)
        return qobject_cast<StateTable *>(parentState->machine());
    if (transition)
        return qobject_cast<StateTable *>(transition->machine());
    qCWarning(scxmlLog) << "Cannot find StateTable of free standing Instruction";
    return 0;
}

QString Instruction::instructionLocation() {
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

void JavaScript::execute()
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
            return;
        }
    }
    qCDebug(scxmlLog) << "executing " << source;
    QJSValue res = compiledFunction.callWithInstance(t->datamodelJSValues());
    if (res.isError()) {
        t->submitError(QByteArray("error.execution"), QStringLiteral("%1 in %2").arg(res.toString(), instructionLocation()));
    }
}

void InstructionVisitor::accept(Instruction *instruction) {
    switch (instruction->instructionKind()) {
    case Instruction::Raise:
        visitRaise(static_cast<Raise *>(instruction));
        break;
    case Instruction::Send:
        visitSend(static_cast<Send *>(instruction));
        break;
    case Instruction::JavaScript:
        visitJavaScript(static_cast<JavaScript *>(instruction));
        break;
    case Instruction::AssignJson:
        visitAssignJson(static_cast<AssignJson *>(instruction));
        break;
    case Instruction::AssignExpression:
        visitAssignExpression(static_cast<AssignExpression *>(instruction));
        break;
    case Instruction::If: {
        auto ifInstruction = static_cast<If *>(instruction);
        if (!visitIf(ifInstruction))
            break;
        for (int iblock = 0; iblock < ifInstruction->blocks.size(); ++iblock) {
            InstructionSequence &block = ifInstruction->blocks[iblock];
            if (visitSequence(&block)) {
                for (int i = 0; i < block.statements.size(); ++i) {
                    accept(block.statements[i].data());
                }
            }
            endVisitSequence(&block);
        }
        endVisitIf(ifInstruction);
        break;
    }
    case Instruction::Foreach: {
        auto foreachInstruction = static_cast<Foreach *>(instruction);
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
        visitLog(static_cast<Log *>(instruction));
        break;
    case Instruction::Cancel:
        visitCancel(static_cast<Cancel *>(instruction));
        break;
    case Instruction::Invoke: {
        visitInvoke(static_cast<Invoke *>(instruction));
        auto invokeInstruction = static_cast<Invoke *>(instruction);
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
        auto sequenceInstruction = static_cast<InstructionSequence *>(instruction);
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

void If::execute()
{
    StateTable *t = table();
    for (int i = 0; i < conditions.size(); ++i) {
        if (t->evalValueBool(conditions.at(i), [this]() -> QString { return instructionLocation(); })) {
            blocks[i].execute();
            return;
        }
    }
    if (conditions.size() < blocks.size())
        blocks[conditions.size()].execute();
}

Send::~Send()
{
    delete content; content = 0;
}

} // namespace ExecutableContent

StateTable::StateTable(QObject *parent)
    : QStateMachine(*new StateTablePrivate, parent), m_initialSetup(this), m_dataModel(None)
    , m_engine(0), m_dataBinding(EarlyBinding), m_warnIndirectIdClashes(true)
{ }

StateTable::StateTable(StateTablePrivate &dd, QObject *parent)
    : QStateMachine(dd, parent), m_initialSetup(this), m_dataModel(None), m_engine(0)
    , m_dataBinding(EarlyBinding), m_warnIndirectIdClashes(true)
{ }


bool StateTable::addId(const QByteArray &idStr, QObject *value, std::function<bool (const QString &)> errorDumper, bool overwrite)
{
    if (m_idObjects.contains(idStr)) {
        QObject *oldVal = m_idObjects.value(idStr).data();
        if (overwrite) {
            if (oldVal == value)
                return true;
            m_idObjects.remove(idStr);
            m_objectIds.remove(oldVal);
        } else {
            if (oldVal != value) {
                if (errorDumper) errorDumper(QStringLiteral("duplicate id '%1'")
                                             .arg(QString::fromUtf8(idStr)));
                return true;
            } else {
                if (errorDumper) errorDumper(QStringLiteral("reinsert of id '%1'")
                                             .arg(QString::fromUtf8(idStr)));
                return false;
            }
        }
    }
    if (m_warnIndirectIdClashes) {
        auto oldVal = idToValue<QObject>(idStr, true);
        if (oldVal != 0 && oldVal != value)
            if (errorDumper) errorDumper(QStringLiteral("id '%1' shadows indirectly accessible ")
                                         .arg(QString::fromUtf8(idStr)));
    }
    m_idObjects.insert(idStr, value);
    m_objectIds.insert(value, idStr);
    return true;
}

QJSValue StateTable::datamodelJSValues() const {
    return m_dataModelJSValues;
    // QQmlEngine::​setObjectOwnership
}

QByteArray StateTable::objectId(QObject *obj, bool strict) {
    QByteArray res = m_objectIds.value(obj);
    if (!strict || !res.isEmpty())
        return res;
    if (!obj)
        return QByteArray();
    QString nameAtt = obj->objectName();
    if (!nameAtt.isEmpty()) { // try to use objectname if it does not clash with a subObject
        QObject *subObj = findChild<QObject *>(nameAtt);
        res = nameAtt.toUtf8();
        if ((!subObj || subObj == obj) && m_idObjects.value(res).data() == 0) {
            addId(res, obj, Q_NULLPTR, true);
            return res;
        }
        QObject *objAtt = obj;
        QObject *oldObj = obj;
        QObject *parentAtt = objAtt->parent();
        QStringList path;
        path.prepend(nameAtt);
        while (parentAtt) {
            if (m_objectIds.contains(parentAtt)) {
                QByteArray idStr = m_objectIds.value(parentAtt);
                if (m_idObjects.value(idStr).data() != parentAtt) {
                    m_objectIds.remove(parentAtt);
                    path.clear();
                } else {
                    if (parentAtt->findChild<QObject *>(path.first()) == oldObj)
                        path.prepend(QString::fromUtf8(idStr));
                    else
                        path.clear();
                }
                break;
            } else if (parentAtt == this) {
                if (findChild<QObject *>(path.first()) != oldObj)
                    path.clear();
                break;
            }
            nameAtt = parentAtt->objectName();
            if (!nameAtt.isEmpty()) {
                if (parentAtt->findChild<QObject *>(path.first()) != oldObj) {
                    path.clear();
                    break;
                }
                oldObj = parentAtt;
                path.prepend(nameAtt);
            }
            objAtt = parentAtt;
            parentAtt = objAtt->parent();
        }
        if (!path.isEmpty() && parentAtt)
            return path.join(QLatin1Char('.')).toUtf8(); // add to cache?
    }
    QByteArray idStr = QStringLiteral("@%1").arg((size_t)(void *)obj).toUtf8();
    addId(idStr, obj, Q_NULLPTR, true);
    return idStr;
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

QString StateTable::evalValueStr(const QString &expr, std::function<QString()> context,
                                 const QString &defaultValue)
{
    QJSEngine *e = engine();
    if (e) {
        QJSValue v = e->evaluate(QStringLiteral("(function(){ return (\n%1\n).toString(); })()").arg(expr),
                                 QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            submitError(QByteArray("error.execution"),
                        QStringLiteral("%1 in %2").arg(v.toString(), context()));
        } else {
            return v.toString();
        }
    }
    return defaultValue;
}

int StateTable::evalValueInt(const QString &expr, std::function<QString()> context, int defaultValue)
{
    QJSEngine *e = engine();
    if (e) {
        QJSValue v = e->evaluate(QStringLiteral("(function(){ return (\n%1\n)|0; })()").arg(expr),
                                 QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            submitError(QByteArray("error.execution"),
                        QStringLiteral("%1 in %2").arg(v.toString(), context()));
        } else {
            return v.toInt();
        }
    }
    return defaultValue;
}

bool StateTable::evalValueBool(const QString &expr, std::function<QString()> context, bool defaultValue)
{
    QJSEngine *e = engine();
    if (e) {
        QJSValue v = e->evaluate(QStringLiteral("(function(){ return !!(\n%1\n); })()").arg(expr),
                                 QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            submitError(QByteArray("error.execution"),
                        QStringLiteral("%1 in %2").arg(v.toString(), context()));
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
                    .arg(context(), v.toString(), getData));
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
    qCDebug(scxmlLog) << _name << " started microstep from state (" << currentStates() << ")"
                      << "with event " << _event.name() << " from event type " << event->type();
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
#endif

QList<QByteArray> StateTable::currentStates(bool compress) {
    QSet<QAbstractState *> config = d_func()->configuration;
    if (compress)
        foreach (const QAbstractState *s, d_func()->configuration)
            config.remove(s->parentState());
    QList<QByteArray> res;
    foreach (const QAbstractState *s, config)
        res.append(objectId(const_cast<QAbstractState *>(s)));
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
    foreach (const ScxmlData &data ,m_data) {
        QJSValue v;
        if ((dataBinding() == EarlyBinding || !data.context || data.context == this)
                && !data.expr.isEmpty())
            v = evalJSValue(data.expr, [this, &data]() -> QString {
                return QStringLiteral("setupDataModel with data for %1 defined in state '%2'")
                        .arg(data.id, QString::fromUtf8(objectId(data.context)));
            });
        m_dataModelJSValues.setProperty(data.id, v);
    }
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
    bool res = true;
    loopOnSubStates(this, std::function<bool(QState *)>(), [&res](QState *state) {
        if (ScxmlState *s = qobject_cast<ScxmlState *>(state))
            if (!s->init())
                res = false;
        foreach (QAbstractTransition *t, state->transitions()) {
            if (ScxmlTransition *scTransition = qobject_cast<ScxmlTransition *>(t))
                if (!scTransition->init())
                    res = false;
        }
    });
    setupDataModel();
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

void StateTable::submitEvent(const QByteArray &event, const QVariantList &datas,
                             const QStringList &dataNames, ScxmlEvent::EventType type,
                             const QByteArray &sendid, const QString &origin,
                             const QString &origintype, const QByteArray &invokeid)
{
    ScxmlEvent *e = new ScxmlEvent(event, type, datas, dataNames, sendid, origin, origintype, invokeid);
    postEvent(e);
}

void StateTable::submitDelayedEvent(int delay, const QByteArray &event, const QVariantList &datas,
                                    const QStringList &dataNames, ScxmlEvent::EventType type,
                                    const QByteArray &sendid,
                                    const QString &origin, const QString &origintype,
                                    const QByteArray &invokeid)
{
    ScxmlEvent *e = new ScxmlEvent(event, type, datas, dataNames, sendid, origin, origintype, invokeid);
    postDelayedEvent(e, delay);
}

ScxmlEvent::ScxmlEvent(const QByteArray &name, ScxmlEvent::EventType eventType,
                       const QVariantList &datas, const QStringList &dataNames,
                       const QByteArray &sendid, const QString &origin,
                       const QString &origintype, const QByteArray &invokeid)
    : QEvent(scxmlEventType), m_name(name), m_type(eventType), m_datas(datas), m_dataNames(dataNames)
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

QVariant ScxmlEvent::data() const {
    if (!m_datas.isEmpty()) {
        if (m_datas.length() == 1)
            return m_datas.first();
        return m_datas;
    }
    return QVariant();
}

void ScxmlEvent::reset(const QByteArray &name, ScxmlEvent::EventType eventType, QVariantList datas,
                       const QByteArray &sendid, const QString &origin,
                       const QString &origintype, const QByteArray &invokeid) {
    m_name = name;
    m_type = eventType;
    m_sendid = sendid;
    m_origin = origin;
    m_origintype = origintype;
    m_invokeid = invokeid;
    m_datas = datas;
}

void ScxmlEvent::clear() {
    m_name = QByteArray();
    m_type = External;
    m_sendid = QByteArray();
    m_origin = QString();
    m_origintype = QString();
    m_invokeid = QByteArray();
    m_datas = QVariantList();
}

QJSValue ScxmlEvent::jsValue(QJSEngine *engine) const {
    QJSValue res = engine->newObject();
    res.setProperty(QStringLiteral("data"), engine->toScriptValue(data()));
    if (!invokeid().isEmpty())
        res.setProperty(QStringLiteral("invokeid"), engine->toScriptValue(invokeid()) );
    if (!origintype().isEmpty())
        res.setProperty(QStringLiteral("origintype"), engine->toScriptValue(origintype()) );
    if (!origin().isEmpty())
        res.setProperty(QStringLiteral("origin"), engine->toScriptValue(origin()) );
    if (!sendid().isEmpty())
        res.setProperty(QStringLiteral("sendid"), engine->toScriptValue(sendid()) );
    res.setProperty(QStringLiteral("type"), engine->toScriptValue(scxmlType()) );
    res.setProperty(QStringLiteral("name"), engine->toScriptValue(name()) );
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
    if (sourceState())
        return qobject_cast<StateTable *>(sourceState()->machine());
    qCWarning(scxmlLog) << "could not resolve StateTable in " << transitionLocation();
    return 0;
}

QString ScxmlBaseTransition::transitionLocation() const {
    if (QState *state = sourceState()) {
        QString stateName;
        if (StateTable *stateTable = table())
            stateName = QString::fromUtf8(stateTable->objectId(state, true));
        else
            stateName = state->objectName();
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
    QByteArray eventName = stateTable->_event.name();
    bool selected = false;
    foreach (const QByteArray &eventStr, eventSelector) {
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
    StateTable *stateTable = table();
    bool failure = false;
    foreach (const QByteArray &eventStr, eventSelector) {
        QList<QByteArray> selector = eventStr.split('.');
        if (selector.isEmpty())
            continue;
        else if (selector.first() == QByteArray("qsignal")) {
            if (selector.count() < 2) {
                qCWarning(scxmlLog) << "qeventSelector requires a sender id in " << transitionLocation();
                failure = true;
                continue;
            }
            QObject *sender = stateTable->idToValue<QObject>(selector.value(1));
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
    if (StateTable *t = table()) {
        foreach (QAbstractState *s, targetStates())
            res << t->objectId(s, true);
    }
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
                                 const QList<QByteArray> &targetIds, const QString &conditionalExp) :
    ScxmlBaseTransition(sourceState, filterEmpty(eventSelector)),
    conditionalExp(conditionalExp), instructionsOnTransition(sourceState, this),
    m_targetIds(filterEmpty(targetIds)) { }

bool ScxmlTransition::eventTest(QEvent *event) {
    if (ScxmlBaseTransition::eventTest(event)
            && (conditionalExp.isEmpty()
                || table()->evalValueBool(conditionalExp, [this]() -> QString {
                                          return transitionLocation();
                                          })))
        return true;
    return false;
}

bool ScxmlTransition::init() {
    StateTable *stateTable = table();
    if (!m_targetIds.isEmpty()) {
        QList<QAbstractState *> targets;
        foreach (const QByteArray &tId, m_targetIds) {
            QAbstractState *target = stateTable->idToValue<QAbstractState>(tId);
            if (!target) {
                qCWarning(scxmlLog) << "ScxmlTransition could not resolve target state for id "
                                    << tId << " for " << transitionLocation();
                return false;
            }
            targets << target;
        }
        setTargetStates(targets);
    } else if (!targetStates().isEmpty()) {
        setTargetStates(QList<QAbstractState *>()); // avoid?
    }
    return ScxmlBaseTransition::init();
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
    if (!onEntryInstruction.init())
        return false;
    if (!onExitInstruction.init())
        return false;
    return true;
}

QString ScxmlState::stateLocation() const
{
    StateTable *t = table();
    QString id = (t ? QString::fromUtf8(t->objectId(const_cast<ScxmlState *>(this), true)) : QString());
    QString name = objectName();
    return QStringLiteral("State %1 (%2)").arg(id, name);
}

void ScxmlState::onEntry(QEvent *event) {
    if (!m_dataInitialized) {
        m_dataInitialized = true;
        // this might actually be a bit too late (parallel states might already have been entered)
        table()->initializeDataFor(this);
    }
    QState::onEntry(event);
    onEntryInstruction.execute();
}

void ScxmlState::onExit(QEvent *event) {
    QState::onExit(event);
    onExitInstruction.execute();
}

StateTable *ScxmlFinalState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlFinalState::init()
{
    if (!onEntryInstruction.init())
        return false;
    if (!onExitInstruction.init())
        return false;
    return true;
}

void ScxmlFinalState::onEntry(QEvent *event) {
    QFinalState::onEntry(event);
    onEntryInstruction.execute();
}

void ScxmlFinalState::onExit(QEvent *event) {
    QFinalState::onExit(event);
    onExitInstruction.execute();
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

bool ScxmlInitialState::init()
{
    bool res = true;
    if (transitions().size() != 1) {
        qCWarning(scxmlLog) << "An initial state (like " << stateLocation() << " in "
                            << (parentState() ? parentState()->objectName() : QStringLiteral("*null*"))
                            << "should have exactly one transition.";
        res = false;
    } else if (ScxmlTransition *t = qobject_cast<ScxmlTransition *>(transitions().first())){
        if (!t->eventSelector.isEmpty() || !t->conditionalExp.isEmpty()) {
            qCWarning(scxmlLog) << "A transition of an initial state (like " << stateLocation() << " in "
                                << (parentState() ? parentState()->objectName() : QStringLiteral("*null*"))
                                << ") should have no event or condition";
        }
    }
    foreach (QObject *child, children()) {
        if (qobject_cast<QState *>(child)) {
            qCWarning(scxmlLog) << "An initial state (like " << stateLocation() << " in "
                                << (parentState() ? parentState()->objectName() : QStringLiteral("*null*"))
                                << "should have no substates.";
            res = false;
            break;
        }
    }
    return res;
}

} // namespace Scxml
