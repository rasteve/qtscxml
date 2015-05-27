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
#include "executablecontent_p.h"
#include "scxmlevent_p.h"

#include <QAbstractState>
#include <QAbstractTransition>
#include <QState>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QLoggingCategory>
#include <QJSEngine>
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

} // anonymous namespace

QAtomicInt StateTablePrivate::m_sessionIdCounter = QAtomicInt(0);

StateTablePrivate::StateTablePrivate()
    : QStateMachinePrivate()
    , m_sessionId(m_sessionIdCounter++)
{
    Q_Q(StateTable);

    m_executionEngine = new ExecutableContent::ExecutionEngine(q);
}

StateTablePrivate::~StateTablePrivate()
{
    delete m_executionEngine;
    delete m_queuedEvents;
}

StateTable::StateTable(QObject *parent)
    : QStateMachine(*new StateTablePrivate, parent)
{
    connect(this, &QStateMachine::finished, this, &StateTable::onFinished);
}

StateTable::StateTable(StateTablePrivate &dd, QObject *parent)
    : QStateMachine(dd, parent)
{
    connect(this, &QStateMachine::finished, this, &StateTable::onFinished);
}

StateTablePrivate *StateTable::privateData()
{
    Q_D(StateTable);

    return d;
}

int StateTable::sessionId() const
{
    Q_D(const StateTable);

    return d->m_sessionId;
}

DataModel *StateTable::dataModel() const
{
    Q_D(const StateTable);

    return d->m_dataModel;
}

void StateTable::setDataModel(DataModel *dataModel)
{
    Q_D(StateTable);

    d->m_dataModel = dataModel;
}

void StateTable::setDataBinding(StateTable::BindingMethod b)
{
    Q_D(StateTable);

    d->m_dataBinding = b;
}

StateTable::BindingMethod StateTable::dataBinding() const
{
    Q_D(const StateTable);

    return d->m_dataBinding;
}

ExecutableContent::ExecutionEngine *StateTable::executionEngine() const
{
    Q_D(const StateTable);

    return d->m_executionEngine;
}

void StateTable::doLog(const QString &label, const QString &msg)
{
    qCDebug(scxmlLog) << label << ":" << msg;
    emit log(label, msg);
}

void StateTable::beginSelectTransitions(QEvent *event)
{
    Q_D(StateTable);

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
                    d->_event.reset(signalName.mid(6), eventType, e->arguments());
                    break;
                } else {
                    qCWarning(scxmlLog) << "Unexpected signal event sent to StateMachine "
                                        << d->_name << ":" << signalName;
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
            d->_event.reset(eventName, eventType, e->arguments());
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
            d->_event.reset(name, eventType); // put something more in data for some elements like keyEvents and mouseEvents?
        } break;
        default:
            if (event->type() == ScxmlEvent::scxmlEventType) {
                d->_event = *static_cast<ScxmlEvent *>(event);
            } else {
                QEvent::Type qeventType = event->type();
                QByteArray eventName = QStringLiteral("qdirectevent.E%1").arg((int)qeventType).toUtf8();
                d->_event.reset(eventName);
                qCWarning(scxmlLog) << "Unexpected event directly sent to StateMachine "
                                    << d->_name << ":" << event->type();
            }
            break;
        }
    } else {
        d->_event.clear();
    }

    dataModel()->setEvent(d->_event);
}

void StateTable::beginMicrostep(QEvent *event)
{
    Q_D(StateTable);

    qCDebug(scxmlLog) << d->_name << " started microstep from state" << currentStates()
                      << "with event" << d->_event.name() << "from event type" << event->type();
}

void StateTable::endMicrostep(QEvent *event)
{
    Q_D(StateTable);
    Q_UNUSED(event);

    qCDebug(scxmlLog) << d->_name << " finished microstep in state (" << currentStates() << ")";
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

void StateTablePrivate::noMicrostep()
{
    Q_Q(StateTable);

    qCDebug(scxmlLog) << _name << " had no transition, stays in state (" << q->currentStates() << ")";
}

void StateTablePrivate::processedPendingEvents(bool didChange)
{
    Q_Q(StateTable);

    qCDebug(scxmlLog) << _name << " finishedPendingEvents " << didChange << " in state ("
                      << q->currentStates() << ")";
    emit q->reachedStableState(didChange);
}

void StateTablePrivate::beginMacrostep()
{
}

void StateTablePrivate::endMacrostep(bool didChange)
{
    Q_Q(StateTable);
    qCDebug(scxmlLog) << _name << " endMacrostep " << didChange << " in state ("
                      << q->currentStates() << ")";
}

void StateTablePrivate::emitStateFinished(QState *forState, QFinalState *guiltyState)
{
    Q_Q(StateTable);

    if (ScxmlFinalState *finalState = qobject_cast<ScxmlFinalState *>(guiltyState)) {
        if (!q->isRunning())
            return;
        q->executionEngine()->execute(finalState->doneData(), forState->objectName());
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

void StateTable::setName(const QString &name)
{
    Q_D(StateTable);

    d->_name = name;
}

void StateTable::setInitialSetup(ExecutableContent::ContainerId sequence)
{
    Q_D(StateTable);

    d->m_initialSetup = sequence;
}

void StateTable::setDataItemNames(const ExecutableContent::StringIds &dataItemNames)
{
    Q_D(StateTable);

    d->dataItemNames = dataItemNames;
}

void StateTable::executeInitialSetup()
{
    Q_D(StateTable);

    executionEngine()->execute(d->m_initialSetup);
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
    Q_D(StateTable);
    dataModel()->setup(d->dataItemNames);
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
    Q_D(const StateTable);

    return d->m_engine;
}

void StateTable::setEngine(QJSEngine *engine)
{
    Q_D(StateTable);

    d->m_engine = engine;
}

QString StateTable::name() const
{
    Q_D(const StateTable);

    return d->_name;
}

void StateTable::submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid)
{
    Q_D(StateTable);

    qCDebug(scxmlLog) << "machine" << d->_name << "had error" << type << ":" << msg;
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
    Q_D(StateTable);

    qCDebug(scxmlLog) << d->_name << ": submitting event" << event;

    ScxmlEvent *e = new ScxmlEvent(event, type, dataValues, dataNames, sendid, origin, origintype, invokeid);
    submitEvent(e);
}

void StateTable::submitDelayedEvent(int delayInMiliSecs, ScxmlEvent *e)
{
    Q_ASSERT(delayInMiliSecs > 0);
    Q_D(StateTable);

    if (!e)
        return;

    qCDebug(scxmlLog) << d->_name << ": submitting event" << e->name() << "with delay" << delayInMiliSecs << "ms" << "and sendid" << e->sendid();

    Q_ASSERT(e->eventType() == ScxmlEvent::External);
    int id = postDelayedEvent(e, delayInMiliSecs);

    qCDebug(scxmlLog) << d->_name << ": delayed event" << e->name() << "(" << e << ") got id:" << id;
}

void StateTable::cancelDelayedEvent(const QByteArray &sendid)
{
    Q_D(StateTable);

    int id = d->eventIdForDelayedEvent(sendid);

    qCDebug(scxmlLog) << name() << ": canceling event" << sendid << "with id" << id;

    if (id != -1)
        QStateMachine::cancelDelayedEvent(id);
}

void StateTable::queueEvent(ScxmlEvent *event, EventPriority priority)
{
    Q_D(StateTable);

    qCDebug(scxmlLog) << name() << ": queueing event" << event->name();

    if (!d->m_queuedEvents)
        d->m_queuedEvents = new QVector<StateTablePrivate::QueuedEvent>();
    d->m_queuedEvents->append({event, priority});
}

void StateTable::submitQueuedEvents()
{
    Q_D(StateTable);

    qCDebug(scxmlLog) << name() << ": submitting queued events";

    if (d->m_queuedEvents) {
        foreach (const StateTablePrivate::QueuedEvent &e, *d->m_queuedEvents)
            postEvent(e.event, e.priority);
        delete d->m_queuedEvents;
        d->m_queuedEvents = nullptr;
    }
}

bool StateTable::isLegalTarget(const QString &target) const
{
    return target.startsWith(QLatin1Char('#'));
}

bool StateTable::isDispatchableTarget(const QString &target) const
{
    return target == QStringLiteral("#_internal")
            || target == QStringLiteral("#_scxml_%1").arg(sessionId());
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

class ScxmlBaseTransition::Data
{
public:
    QList<QByteArray> eventSelector;
    QList<TransitionPtr> m_concreteTransitions;
};

ScxmlBaseTransition::ScxmlBaseTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : QAbstractTransition(sourceState)
{
    d->eventSelector = eventSelector;
}

ScxmlBaseTransition::ScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                                         const QList<QByteArray> &eventSelector)
    : QAbstractTransition(dd, parent)
    , d(new Data)
{
    d->eventSelector = eventSelector;
}

ScxmlBaseTransition::~ScxmlBaseTransition()
{
    delete d;
}

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

bool ScxmlBaseTransition::eventTest(QEvent *event)
{
    if (d->eventSelector.isEmpty())
        return true;
    if (event->type() == QEvent::None)
        return false;
    StateTable *stateTable = table();
    Q_ASSERT(stateTable);
    QByteArray eventName = stateTable->privateData()->_event.name();
    bool selected = false;
    foreach (QByteArray eventStr, d->eventSelector) {
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
    if (!d->m_concreteTransitions.isEmpty() && event->type() == QEvent::StateMachineSignal
            && static_cast<QStateMachine::SignalEvent *>(event)->sender() != stateTable) {
        bool selected2 = false;
        foreach (TransitionPtr t, d->m_concreteTransitions) {
            if (t->subEventTest(event))
                selected2 = true;
        }
        if (selected != selected2) {
            qCWarning(scxmlLog) << "text based triggering and signal based triggering differs for event"
                                << eventName << " text based comparison with '"
                                << d->eventSelector.join(' ')
                                << "' gives value " << selected
                                << " while the underlying concrete transitions give "
                                << selected2 << " in " << transitionLocation();
        }
    }
#endif
    return selected;
}

bool ScxmlBaseTransition::clear()
{
    foreach (TransitionPtr t, d->m_concreteTransitions)
        sourceState()->removeTransition(t.data());
    d->m_concreteTransitions.clear();
    return true;
}

bool ScxmlBaseTransition::init()
{
    Q_ASSERT(d->m_concreteTransitions.isEmpty());
    if (d->eventSelector.isEmpty())
        return true;
    bool failure = false;
    foreach (const QByteArray &eventStr, d->eventSelector) {
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
                    d->m_concreteTransitions << TransitionPtr(newT);
                }
            }
            if (d->m_concreteTransitions.isEmpty()) {
                QList<QByteArray> knownSignals;
                for (int imethod = 0; imethod < maxImethod; ++imethod){
                    QMetaMethod m = metaObject->method(imethod);
                    if (m.methodType() != QMetaMethod::Signal) continue;
                    QByteArray mName = m.methodSignature();
                    knownSignals.append(mName);
                }
                qCWarning(scxmlLog) << "eventSelector failed to match anything in "
                                    << transitionLocation() << ", selector is: "
                                    << d->eventSelector.join(' ') << " and known signals are:\n  "
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

class ScxmlTransition::Data
{
public:
    EvaluatorId conditionalExp = NoEvaluator;
    ScxmlEvent::EventType type;
    ExecutableContent::ContainerId instructionsOnTransition = ExecutableContent::NoInstruction;
};

ScxmlTransition::ScxmlTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : ScxmlBaseTransition(sourceState, filterEmpty(eventSelector))
    , d(new Data)
{
    Q_ASSERT(sourceState);
    d->type = ScxmlEvent::External;
}

ScxmlTransition::~ScxmlTransition()
{
    delete d;
}

bool ScxmlTransition::eventTest(QEvent *event)
{
        qCDebug(scxmlLog) << qPrintable(table()->engine()->evaluate(QLatin1String("JSON.stringify(_event)")).toString());
    if (ScxmlBaseTransition::eventTest(event)) {
        bool ok = true;
        if (d->conditionalExp != NoEvaluator)
            return table()->dataModel()->evaluateToBool(d->conditionalExp, &ok) && ok;
        return true;
    }

    return false;
}

void ScxmlTransition::onTransition(QEvent *)
{
    table()->executionEngine()->execute(d->instructionsOnTransition);
}

StateTable *ScxmlTransition::table() const {
    // work around a bug in QStateMachine
    if (StateTable *t = qobject_cast<StateTable *>(sourceState()))
        return t;
    return qobject_cast<StateTable *>(machine());
}

void ScxmlTransition::setInstructionsOnTransition(ExecutableContent::ContainerId instructions)
{
    d->instructionsOnTransition = instructions;
}

void ScxmlTransition::setConditionalExpression(EvaluatorId evaluator)
{
    d->conditionalExp = evaluator;
}

class ScxmlState::Data
{
public:
    ExecutableContent::ContainerId initInstructions = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onEntryInstructions = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onExitInstructions = ExecutableContent::NoInstruction;
};

ScxmlState::ScxmlState(QState *parent)
    : QState(parent)
    , d(new Data)
{}

ScxmlState::~ScxmlState()
{
    delete d;
}

StateTable *ScxmlState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlState::init()
{
    return true;
}

QString ScxmlState::stateLocation() const
{
    return QStringLiteral("State %1").arg(objectName());
}

void ScxmlState::setInitInstructions(ExecutableContent::ContainerId instructions)
{
    d->initInstructions = instructions;
}

void ScxmlState::setOnEntryInstructions(ExecutableContent::ContainerId instructions)
{
    d->onEntryInstructions = instructions;
}

void ScxmlState::setOnExitInstructions(ExecutableContent::ContainerId instructions)
{
    d->onExitInstructions = instructions;
}

ScxmlState::ScxmlState(QStatePrivate &dd, QState *parent)
    : QState(dd, parent)
{}

void ScxmlState::onEntry(QEvent *event)
{
    if (d->initInstructions != ExecutableContent::NoInstruction) {
        table()->executionEngine()->execute(d->initInstructions);
        d->initInstructions = ExecutableContent::NoInstruction;
    }
    QState::onEntry(event);
    table()->executionEngine()->execute(d->onEntryInstructions);
}

void ScxmlState::onExit(QEvent *event)
{
    QState::onExit(event);
    table()->executionEngine()->execute(d->onExitInstructions);
}

class ScxmlFinalState::Data
{
public:
    ExecutableContent::ContainerId doneData = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onEntryInstructions = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onExitInstructions = ExecutableContent::NoInstruction;
};

ScxmlFinalState::ScxmlFinalState(QState *parent)
    : QFinalState(parent)
    , d(new Data)
{}

ScxmlFinalState::~ScxmlFinalState()
{
    delete d;
}

StateTable *ScxmlFinalState::table() const {
    return qobject_cast<StateTable *>(machine());
}

bool ScxmlFinalState::init()
{
    return true;
}

Scxml::ExecutableContent::ContainerId ScxmlFinalState::doneData() const
{
    return d->doneData;
}

void ScxmlFinalState::setDoneData(Scxml::ExecutableContent::ContainerId doneData)
{
    d->doneData = doneData;
}

void ScxmlFinalState::setOnEntryInstructions(ExecutableContent::ContainerId instructions)
{
    d->onEntryInstructions = instructions;
}

void ScxmlFinalState::setOnExitInstructions(ExecutableContent::ContainerId instructions)
{
    d->onExitInstructions = instructions;
}

void ScxmlFinalState::onEntry(QEvent *event)
{
    QFinalState::onEntry(event);
    table()->executionEngine()->execute(d->onEntryInstructions);
}

void ScxmlFinalState::onExit(QEvent *event)
{
    QFinalState::onExit(event);
    table()->executionEngine()->execute(d->onExitInstructions);
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

} // namespace Scxml
