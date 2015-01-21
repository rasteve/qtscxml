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

#include "scxmlstatetable.h"

#include <QAbstractState>
#include <QAbstractTransition>
#include <QState>
#include <QHash>
#include <QString>
#include <QLoggingCategory>
#include <QJSEngine>

namespace Scxml {
SCXML_EXPORT Q_LOGGING_CATEGORY(scxmlLog, "scxml.table")

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
        compiledFunction = e->evaluate(QStringLiteral("function() {\n%1\n}").arg(source),
                                       QStringLiteral("<%1>").arg(instructionLocation()), 0);
        if (!compiledFunction.isCallable()) {
            qWarning(scxmlLog) << "Error compiling" << instructionLocation() << " ignoring execution";
            return;
        }
    }
    QJSValue res = compiledFunction.callWithInstance(t->datamodelJSValues());
    if (res.isError()) {
        //t->error()
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
    case Instruction::Invoke:
        visitInvoke(static_cast<Invoke *>(instruction));
        break;
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
        if (t->evalBool(conditions.at(i))) {
            blocks[i].execute();
            return;
        }
    }
    if (conditions.size() < blocks.size())
        blocks[conditions.size()].execute();
}

} // namespace ExecutableContent

StateTable::StateTable(QState *parent)
    : QStateMachine(parent), m_dataModel(None), m_engine(0), m_dataBinding(EarlyBinding), m_warnIndirectIdClashes(true)  { }

bool StateTable::addId(const QString &idStr, QObject *value, std::function<bool (const QString &)> errorDumper, bool overwrite)
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
                if (errorDumper) errorDumper(QStringLiteral("duplicate id '%1'").arg(idStr));
                return true;
            } else {
                if (errorDumper) errorDumper(QStringLiteral("reinsert of id '%1'").arg(idStr));
                return false;
            }
        }
    }
    if (m_warnIndirectIdClashes) {
        auto oldVal = idToValue<QObject>(idStr);
        if (oldVal != 0 && oldVal != value)
            if (errorDumper) errorDumper(QStringLiteral("id '%1' shadows indirectly accessible ").arg(idStr));
    }
    m_idObjects.insert(idStr, value);
    m_objectIds.insert(value, idStr);
    return true;
}

QJSValue StateTable::datamodelJSValues() const {
    return m_dataModelJSValues;
    // QQmlEngine::​setObjectOwnership
}

QString StateTable::objectId(QObject *obj, bool strict) {
    QString res = m_objectIds.value(obj);
    if (!strict || !res.isEmpty())
        return res;
    if (!obj)
        return QString();
    QString nameAtt = obj->objectName();
    if (!nameAtt.isEmpty()) { // try to use objectname if it does not clash with a subObject
        QObject *subObj = findChild<QObject *>(nameAtt);
        if ((!subObj || subObj == obj) && m_idObjects.value(nameAtt).data() == 0) {
            addId(nameAtt, obj, 0, true);
            return nameAtt;
        }
        QObject *objAtt = obj;
        QObject *oldObj = obj;
        QObject *parentAtt = objAtt->parent();
        QStringList path;
        path.prepend(nameAtt);
        while (parentAtt) {
            if (m_objectIds.contains(parentAtt)) {
                QString idStr = m_objectIds.value(parentAtt);
                if (m_idObjects.value(idStr).data() != parentAtt) {
                    m_objectIds.remove(parentAtt);
                    path.clear();
                } else {
                    if (parentAtt->findChild<QObject *>(path.first()) == oldObj)
                        path.prepend(idStr);
                    else
                        path.clear();
                    break;
                }
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
            return path.join(QLatin1Char('.'));
    }
    QString idStr = QString(QStringLiteral("@%1").arg((size_t)(void *)obj));
    addId(idStr, obj, 0, true);
    return idStr;
}

void StateTable::doLog(const QString &label, const QString &msg)
{
    qCDebug(scxmlLog) << label << ":" << msg;
    emit log(label, msg);
}

QString StateTable::evalValueStr(const QString &expr)
{
    return expr;
}

bool StateTable::evalBool(const QString &expr) const
{
    return expr.isEmpty(); // to do
}


void StateTable::beginSelectTransitions(QEvent *event)
{
    if (event && event->type() != QEvent::None) {
        switch (event->type()) {
        case QEvent::StateMachineSignal: {
            QStateMachine::SignalEvent* e = (QStateMachine::SignalEvent*)event;
            QString signalName = QString::fromUtf8(e->sender()->metaObject()->method(e->signalIndex()).methodSignature());
            signalName.replace(signalName.indexOf(QLatin1Char('(')), 1, QLatin1Char('.'));
            signalName.chop(1);
            if (signalName.endsWith(QLatin1Char('.')))
                signalName.chop(1);
            ScxmlEvent::EventType eventType = ScxmlEvent::External;
            QObject *s = e->sender();
            if (s == this) {
                if (signalName.startsWith(QLatin1String("submitEvent"))) {
                    QVariantList args = e->arguments();
                    QString eventName = args[0].toString();
                    eventType = (ScxmlEvent::EventType)args[2].toInt();
                    _event.reset(eventName, eventType, QVariantList() << args[1],
                            /* m_sendid   */ args[3].toString(),
                            /* origin     */ args[4].toString(),
                            /* origintype */ args[5].toString(),
                            /* invokeid   */ args[6].toString());
                    break;
                } else if (signalName.startsWith("event_")){
                    _event.reset(signalName.mid(6), eventType, e->arguments());
                    break;
                } else {
                    qCWarning(scxmlLog) << "Unexpected signal event sent to StateMachine "
                                        << _name << ":" << signalName;
                }
            }
            QString senderName = QStringLiteral("@0");
            if (s) {
                senderName = s->objectName();
                QString uniqueName = objectId(s);
                if (senderName.isEmpty()) // keeps the object name even if not unique, change to the unique string?
                    senderName = uniqueName;
            }
            QString eventName = QStringLiteral("qsignal.%1.%2").arg(senderName, signalName);
            _event.reset(eventName, eventType, e->arguments());
        } break;
        case QEvent::StateMachineWrapped: {
            QStateMachine::WrappedEvent * e = (QStateMachine::WrappedEvent *)event;
            QObject *s = e->object();
            QString senderName = QStringLiteral("@0");
            if (s) {
                senderName = s->objectName();
                QString uniqueName = objectId(s);
                if (senderName.isEmpty()) // keeps the object name even if not unique, change to the unique string?
                    senderName = uniqueName;
            }
            QEvent::Type qeventType = e->event()->type();
            QString eventName;
            QMetaObject metaObject = QEvent::staticMetaObject;
            int maxIenum = metaObject.enumeratorCount();
            for (int ienum = metaObject.enumeratorOffset(); ienum < maxIenum; ++ienum) {
                QMetaEnum en = metaObject.enumerator(ienum);
                if (QLatin1String(en.name()) == QLatin1String("Type")) {
                    eventName = QLatin1String(en.valueToKey(qeventType));
                    break;
                }
            }
            if (eventName.isEmpty())
                eventName = QStringLiteral("E%1").arg((int)qeventType);
            QString name = QStringLiteral("qevent.%1.%2").arg(senderName, eventName);
            ScxmlEvent::EventType eventType = ScxmlEvent::External;
            // use e->spontaneous(); to choose internal/external?
            _event.reset(name, eventType); // put something more in data for some elements like keyEvents and mouseEvents?
        } break;
        default:
            if (event->type() == ScxmlEvent::scxmlEventType) {
                _event = *static_cast<ScxmlEvent *>(event);
            } else {
                QEvent::Type qeventType = event->type();
                QString eventName = QStringLiteral("qdirectevent.E%1").arg((int)qeventType);
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

void StateTable::assignEvent() {
    if (m_engine)
        m_dataModelJSValues.setProperty(QStringLiteral("_event"), _event.jsValue(m_engine));
}

bool StateTable::init(QJSEngine *engine, ErrorDumper)
{
    setEngine(engine);
    QList<int> pos;
    QState *parentAtt = this;
    QObjectList childs = children();
    pos << 0;
    bool res = true;
    while (!pos.isEmpty()) {
        bool goingDeeper = false;
        for (int i = pos.last(); i < childs.size() ; ++i) {
            if (QState *s = qobject_cast<QState *>(childs.at(i))) {
                pos.last() = i + 1;
                parentAtt = s;
                childs = s->children();
                pos << 0;
                goingDeeper = ! childs.isEmpty();
                break;
            }
        }
        if (!goingDeeper) {
            do {
                if (ScxmlState *s = qobject_cast<ScxmlState *>(parentAtt))
                    if (!s->init())
                        res = false;
                foreach (QAbstractTransition *t, parentAtt->transitions()) {
                    if (ScxmlTransition *scTransition = qobject_cast<ScxmlTransition *>(t))
                        scTransition->init();
                }
                parentAtt = parentAtt->parentState();
                pos.removeLast();
                childs = parentAtt->children();
            } while (!pos.isEmpty() && pos.last() >= childs.size());
        }
    }
    return true;
}

QJSEngine *StateTable::engine() const
{
    return m_engine;
}

void StateTable::setEngine(QJSEngine *engine)
{
    m_engine = engine;
}

ScxmlEvent::ScxmlEvent(QString name, ScxmlEvent::EventType eventType, QVariantList datas, const QString &sendid, const QString &origin, const QString &origintype, const QString &invokeid)
    : QEvent(scxmlEventType), m_name(name), m_type(eventType), m_datas(datas),
      m_sendid(sendid), m_origin(origin), m_origintype(origintype), m_invokeid(invokeid)
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

void ScxmlEvent::reset(QString name, ScxmlEvent::EventType eventType, QVariantList datas, const QString &sendid, const QString &origin, const QString &origintype, const QString &invokeid) {
    m_name = name;
    m_type = eventType;
    m_sendid = sendid;
    m_origin = origin;
    m_origintype = origintype;
    m_invokeid = invokeid;
    m_datas = datas;
}

void ScxmlEvent::clear() {
    m_name = QString();
    m_type = External;
    m_sendid = QString();
    m_origin = QString();
    m_origintype = QString();
    m_invokeid = QString();
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

ScxmlTransition::ScxmlTransition(QState *sourceState, const QString &eventSelector, const QString &targetId, const QString &conditionalExp, ExecutableContent::Instruction *instruction) :
    QAbstractTransition(sourceState), eventSelector(eventSelector),
    instructionOnTransition(instruction), conditionalExp(conditionalExp),
    m_targetId(targetId), m_bound(false) { }

StateTable *ScxmlTransition::table() const {
    if (sourceState())
        return qobject_cast<StateTable *>(sourceState()->machine());
    qCWarning(scxmlLog) << "could not resolve StateTable in " << transitionLocation();
    return 0;
}

QString ScxmlTransition::transitionLocation() const {
    if (QState *state = sourceState()) {
        QString stateName = state->objectName();
        if (StateTable *stateTable = table())
            stateName = stateTable->objectId(state, true);
        int transitionIndex = state->transitions().indexOf(const_cast<ScxmlTransition *>(this));
        return QStringLiteral("transition #%1 in state %2").arg(transitionIndex).arg(stateName);
    }
    return QStringLiteral("unbound transition @%1").arg((size_t)(void*)this);
}

bool ScxmlTransition::eventTest(QEvent *event) {
    if (eventSelector.isEmpty())
        return true; // eventless transition, triggers even with QEvent::None
    if (event->type() == QEvent::None)
        return false;
    StateTable *stateTable = table();
    QString eventName = stateTable->_event.name();
    bool selected = false;
    if (eventName.startsWith(eventSelector)) {
        if (event->type() != QEvent::StateMachineSignal && event->type() != ScxmlEvent::scxmlEventType) {
            qCWarning(scxmlLog) << "unexpected triggering of event " << eventName
                                << " with type " << event->type() << " detected in "
                                << transitionLocation();
        }
        QChar nextC = QLatin1Char('.');
        if (eventName.size() < eventSelector.size())
            nextC = eventName.at(eventSelector.size());
        if (nextC == QLatin1Char('.') || nextC == QLatin1Char('('))
            selected = true;
    }
    if (!m_concreteTransitions.isEmpty() && event->type() == QEvent::StateMachineSignal
            && static_cast<QStateMachine::SignalEvent *>(event)->sender() != stateTable) {
        bool selected2 = false;
        foreach (TransitionPtr t, m_concreteTransitions) {
            if (t->subEventTest(event))
                selected2 = true;
        }
        if (selected != selected2) {
            qCWarning(scxmlLog) << "text based triggering and signal based triggering differs for event"
                                << eventName << " text based comparison with " << eventSelector
                                << " gives value " << selected
                                << " while the underlying concrete transitions give "
                                << selected2 << " in " << transitionLocation();
        }
    }
    if (selected && !conditionalExp.isEmpty() && !stateTable->evalBool(conditionalExp))
        selected = false;
    return selected;
}

bool ScxmlTransition::clear() {
    foreach (TransitionPtr t, m_concreteTransitions)
        sourceState()->removeTransition(t.data());
    m_concreteTransitions.clear();
    return true;
}

bool ScxmlTransition::init() {
    StateTable *stateTable = table();
    if (!m_targetId.isEmpty()) {
        QAbstractState *target = stateTable->idToValue<QAbstractState>(m_targetId);
        if (!target) {
            qCWarning(scxmlLog) << "ScxmlTransition could not resolve target state for id "
                                << m_targetId << " for " << transitionLocation();
            return false;
        }
        setTargetState(target);
    } else if (targetState()) {
        setTargetState(0); // avoid?
    }
    if (stateTable->dataBinding() == StateTable::EarlyBinding) {
        m_bound = true;
        return bind();
    }
    return true;
}

bool ScxmlTransition::bind() {
    Q_ASSERT(m_concreteTransitions.isEmpty());
    if (eventSelector.isEmpty())
        return true;
    StateTable *stateTable = table();
    QStringList selector = eventSelector.split(QLatin1Char('.'));
    if (selector.isEmpty())
        return true;
    else if (selector.first() == QLatin1String("qsignal")) {
        if (selector.count() < 2) {
            qCWarning(scxmlLog) << "qeventSelector requires a sender id in " << transitionLocation();
            return false;
        }
        QObject *sender = stateTable->idToValue<QObject>(selector.value(1));
        if (!sender) {
            qCWarning(scxmlLog) << "could not find object with id " << selector.value(1)
                                << " used in " << transitionLocation();
            return false;
        }
        QByteArray methodName = selector.value(2).toUtf8();
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
                                                      && mName.at(methodName.size()) == QLatin1Char('(')))))
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
                                << eventSelector << " and known signals are:\n  "
                                << knownSignals.join(QByteArray("\n  "));
            return false; // ignore instead??
        }
        return true;
    } else if (selector.first() == QLatin1String("qevent")){
        qCWarning(scxmlLog) << "selector of qevent type to implement";
        return false;
    } else {
        // this is expected to be a custom scxml event, no binding required
        return true;
    }
    return true;
}

void ScxmlTransition::onTransition(QEvent *)
{
    StateTable *t = table();
    if (t->dataBinding() != StateTable::EarlyBinding && !m_bound) {
        bind();
        m_bound = t->dataBinding() == StateTable::LateBinding;
    }
}

StateTable *ScxmlState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlState::init()
{
    if (!m_onEntryInstruction.init())
        return false;
    if (!m_onExitInstruction.init())
        return false;
    return true;
}

void ScxmlState::onEntry(QEvent *event) {
    QState::onEntry(event);
    m_onEntryInstruction.execute();
}

void ScxmlState::onExit(QEvent *event) {
    QState::onExit(event);
    m_onExitInstruction.execute();
}

StateTable *ScxmlFinalState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlFinalState::init()
{
    if (!m_onEntryInstruction.init())
        return false;
    if (!m_onExitInstruction.init())
        return false;
    return true;
}

void ScxmlFinalState::onEntry(QEvent *event) {
    QFinalState::onEntry(event);
    m_onEntryInstruction.execute();
}

void ScxmlFinalState::onExit(QEvent *event) {
    QFinalState::onExit(event);
    m_onExitInstruction.execute();
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
    static QRegExp spaceRe("^\\s*$");
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

bool XmlNode::loopOnChilds(std::function<bool (const XmlNode &)> l) {
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

} // namespace Scxml
