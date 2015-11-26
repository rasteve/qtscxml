/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScxml module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#include "qscxmlstatemachine_p.h"
#include "qscxmlexecutablecontent_p.h"
#include "qscxmlevent_p.h"
#include "qscxmlinvokableservice.h"
#include "qscxmlqstates_p.h"
#include "qscxmldatamodel_p.h"

#include <QAbstractState>
#include <QAbstractTransition>
#include <QFile>
#include <QHash>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QState>
#include <QString>
#include <QTimer>

#include <QtCore/private/qstatemachine_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(scxmlLog, "qt.scxml.statemachine")

namespace QScxmlInternal {
class WrappedQStateMachinePrivate: public QStateMachinePrivate
{
    Q_DECLARE_PUBLIC(WrappedQStateMachine)

public:
    WrappedQStateMachinePrivate(QScxmlStateMachine *stateMachine)
        : m_stateMachine(stateMachine)
        , m_queuedEvents(Q_NULLPTR)
    {}
    ~WrappedQStateMachinePrivate()
    {
        if (m_queuedEvents) {
            foreach (const QueuedEvent &qt, *m_queuedEvents) {
                delete qt.event;
            }

            delete m_queuedEvents;
        }
    }

    int eventIdForDelayedEvent(const QByteArray &scxmlEventId);

    QScxmlStateMachine *stateMachine() const
    { return m_stateMachine; }

    QScxmlStateMachinePrivate *stateMachinePrivate() const
    { return QScxmlStateMachinePrivate::get(stateMachine()); }

protected: // overrides for QStateMachinePrivate:
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    void noMicrostep() Q_DECL_OVERRIDE;
    void processedPendingEvents(bool didChange) Q_DECL_OVERRIDE;
    void beginMacrostep() Q_DECL_OVERRIDE;
    void endMacrostep(bool didChange) Q_DECL_OVERRIDE;

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void enterStates(QEvent *event, const QList<QAbstractState*> &exitedStates_sorted,
                     const QList<QAbstractState*> &statesToEnter_sorted,
                     const QSet<QAbstractState*> &statesForDefaultEntry,
                     QHash<QAbstractState *, QVector<QPropertyAssignment> > &propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
                     , const QList<QAbstractAnimation*> &selectedAnimations
#  endif
                     ) Q_DECL_OVERRIDE;
    void exitStates(QEvent *event, const QList<QAbstractState *> &statesToExit_sorted,
                    const QHash<QAbstractState*, QVector<QPropertyAssignment> > &assignmentsForEnteredStates) Q_DECL_OVERRIDE;

    void exitInterpreter() Q_DECL_OVERRIDE;
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)

    void emitStateFinished(QState *forState, QFinalState *guiltyState) Q_DECL_OVERRIDE;
    void startupHook() Q_DECL_OVERRIDE;
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

public: // fields
    QScxmlStateMachine *m_stateMachine;

    struct QueuedEvent
    {
        QueuedEvent(QEvent *event = Q_NULLPTR, QStateMachine::EventPriority priority = QStateMachine::NormalPriority)
            : event(event)
            , priority(priority)
        {}

        QEvent *event;
        QStateMachine::EventPriority priority;
    };
    QVector<QueuedEvent> *m_queuedEvents;
};

WrappedQStateMachine::WrappedQStateMachine(QScxmlStateMachine *parent)
    : QStateMachine(*new WrappedQStateMachinePrivate(parent), parent)
{}

WrappedQStateMachine::WrappedQStateMachine(WrappedQStateMachinePrivate &dd, QScxmlStateMachine *parent)
    : QStateMachine(dd, parent)
{}

QScxmlStateMachine *WrappedQStateMachine::stateMachine() const
{
    Q_D(const WrappedQStateMachine);

    return d->stateMachine();
}

QScxmlStateMachinePrivate *WrappedQStateMachine::stateMachinePrivate()
{
    Q_D(const WrappedQStateMachine);

    return d->stateMachinePrivate();
}
} // Internal namespace

/*!
 * \class QScxmlEventFilter
 * \brief Event filter for a QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * \sa QScxmlStateMachine
 */

QScxmlEventFilter::~QScxmlEventFilter()
{}

/*!
 * \class QScxmlStateMachine
 * \brief Event filter for a QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * QScxmlStateMachine is an implementation of
 * \l{http://www.w3.org/TR/scxml/}{State Chart XML (SCXML)}.
 */

QAtomicInt QScxmlStateMachinePrivate::m_sessionIdCounter = QAtomicInt(0);


QScxmlStateMachinePrivate::QScxmlStateMachinePrivate()
    : QObjectPrivate()
    , m_sessionId(QScxmlStateMachine::generateSessionId(QStringLiteral("session-")))
    , m_isInvoked(false)
    , m_dataModel(Q_NULLPTR)
    , m_dataBinding(QScxmlStateMachine::EarlyBinding)
    , m_executionEngine(Q_NULLPTR)
    , m_tableData(Q_NULLPTR)
    , m_qStateMachine(Q_NULLPTR)
    , m_eventFilter(Q_NULLPTR)
    , m_parentStateMachine(Q_NULLPTR)
{}

QScxmlStateMachinePrivate::~QScxmlStateMachinePrivate()
{
    qDeleteAll(m_invokedServices);
    delete m_executionEngine;
}

void QScxmlStateMachinePrivate::setQStateMachine(QScxmlInternal::WrappedQStateMachine *stateMachine)
{
    m_qStateMachine = stateMachine;
}

static QScxmlState *findState(const QString &scxmlName, QStateMachine *parent)
{
    QList<QObject *> worklist;
    worklist.reserve(parent->children().size() + parent->configuration().size());
    worklist.append(parent);

    while (!worklist.isEmpty()) {
        QObject *obj = worklist.takeLast();
        if (QScxmlState *state = qobject_cast<QScxmlState *>(obj)) {
            if (state->objectName() == scxmlName)
                return state;
        }
        worklist.append(obj->children());
    }

    return Q_NULLPTR;
}

QAbstractState *QScxmlStateMachinePrivate::stateByScxmlName(const QString &scxmlName)
{
    return findState(scxmlName, m_qStateMachine);
}

QScxmlStateMachinePrivate::ParserData *QScxmlStateMachinePrivate::parserData()
{
    if (m_parserData.isNull())
        m_parserData.reset(new ParserData);
    return m_parserData.data();
}

void QScxmlStateMachinePrivate::addService(QScxmlInvokableService *service)
{
    Q_Q(QScxmlStateMachine);
    Q_ASSERT(!m_invokedServices.contains(service));
    m_invokedServices.append(service);
    q->setService(service->name(), service);
}

bool QScxmlStateMachinePrivate::removeService(QScxmlInvokableService *service)
{
    Q_Q(QScxmlStateMachine);
    Q_ASSERT(m_invokedServices.contains(service));
    if (m_invokedServices.removeOne(service)) {
        q->setService(service->name(), Q_NULLPTR);
        return true;
    }
    return false;
}

void QScxmlStateMachinePrivate::executeInitialSetup()
{
    m_executionEngine->execute(m_tableData->initialSetup());
}

QScxmlStateMachine *QScxmlStateMachine::fromFile(const QString &fileName, QScxmlDataModel *dataModel)
{
    QFile scxmlFile(fileName);
    if (!scxmlFile.open(QIODevice::ReadOnly)) {
        auto stateMachine = new QScxmlStateMachine;
        QScxmlError err(scxmlFile.fileName(), 0, 0, QStringLiteral("cannot open for reading"));
        QScxmlStateMachinePrivate::get(stateMachine)->parserData()->m_errors.append(err);
        return stateMachine;
    }

    QScxmlStateMachine *stateMachine = fromData(&scxmlFile, fileName, dataModel);
    scxmlFile.close();
    return stateMachine;
}

QScxmlStateMachine *QScxmlStateMachine::fromData(QIODevice *data, const QString &fileName, QScxmlDataModel *dataModel)
{
    QXmlStreamReader xmlReader(data);
    QScxmlParser parser(&xmlReader);
    parser.setFileName(fileName);
    parser.parse();
    auto stateMachine = parser.instantiateStateMachine();
    if (dataModel) {
        stateMachine->setDataModel(dataModel);
    } else {
        parser.instantiateDataModel(stateMachine);
    }
    return stateMachine;
}

QVector<QScxmlError> QScxmlStateMachine::errors() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_parserData ? d->m_parserData->m_errors : QVector<QScxmlError>();
}

/*!
 * Constructs a new state machine with the given parent.
 */
QScxmlStateMachine::QScxmlStateMachine(QObject *parent)
    : QObject(*new QScxmlStateMachinePrivate, parent)
{
    Q_D(QScxmlStateMachine);
    d->m_executionEngine = new QScxmlExecutableContent::QScxmlExecutionEngine(this);
    d->setQStateMachine(new QScxmlInternal::WrappedQStateMachine(this));
    connect(d->m_qStateMachine, &QStateMachine::runningChanged, this, &QScxmlStateMachine::runningChanged);
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::finished);
    connect(d->m_qStateMachine, &QStateMachine::finished, [this](){
        // The final state is also a stable state.
        emit reachedStableState(true);
    });
}

/*!
 * \internal
 */
QScxmlStateMachine::QScxmlStateMachine(QScxmlStateMachinePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QScxmlStateMachine);
    d->m_executionEngine = new QScxmlExecutableContent::QScxmlExecutionEngine(this);
    connect(d->m_qStateMachine, &QStateMachine::runningChanged, this, &QScxmlStateMachine::runningChanged);
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::finished);
    connect(d->m_qStateMachine, &QStateMachine::finished, [this](){
        // The final state is also a stable state.
        emit reachedStableState(true);
    });
}

/*!
    \property QScxmlStateMachine::running

    \brief the running state of this state machine

    \sa start(), runningChanged()
 */

/*!
    \enum QScxmlStateMachine::BindingMethod

    This enum specifies the binding method. The binding method controls when the initial values are
    assigned to the data elements.

    \value EarlyBinding will create and initialize all data elements at data-model initialization.
           This is the default.
    \value LateBinding will create all data elements at initialization, but only assign the initial
           values when the containing state is entered for the first time. It will do so before any
           executable content is executed.
 */

/*!
 * \brief Returns the session ID for the current state machine.
 *
 * The session ID is used for message routing between parent and child state machines. If a state
 * machine is started by <invoke>, any event it sends will have the invokeid field of set to the
 * session ID. The state machine will use the origin of an event (which is set by the target or
 * targetexpr attribute in a <send> tag) to dispatch a message from one state machine to the correct
 * child state machine.
 *
 * \sa QScxmlStateMachine::setSessionId() QScxmlEvent::invokeId() QScxmlStateMachine::routeEvent()
 */
QString QScxmlStateMachine::sessionId() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_sessionId;
}

/*!
  Sets the session ID for the current state machine.

  \sa QScxmlStateMachine::sessionId
 */
void QScxmlStateMachine::setSessionId(const QString &id)
{
    Q_D(QScxmlStateMachine);
    d->m_sessionId = id;
}

/*!
 * \brief Generates a unique ID by appending a unique number to the prefix.
 *
 * The number is only unique within a single run of an application. This method is used when an
 * invoked service does not have an ID set (the id attribute in <invoke>).
 *
 * \param prefix The prefix onto which a unique number is appended.
 */
QString QScxmlStateMachine::generateSessionId(const QString &prefix)
{
    int id = ++QScxmlStateMachinePrivate::m_sessionIdCounter;
    return prefix + QString::number(id);
}

/*!
 * \return true when the state machine was started as a service with <invoke>, false otherwise.
 */
bool QScxmlStateMachine::isInvoked() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_isInvoked;
}

/*!
 * \return Returns the data-model used by the state-machine.
 */
QScxmlDataModel *QScxmlStateMachine::dataModel() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_dataModel;
}

void QScxmlStateMachine::setDataModel(QScxmlDataModel *dataModel)
{
    Q_D(QScxmlStateMachine);

    d->m_dataModel = dataModel;
    QScxmlDataModelPrivate::get(dataModel)->setStateMachine(this);
}

/*!
 * Sets the binding method to the specified value.
 */
void QScxmlStateMachine::setDataBinding(QScxmlStateMachine::BindingMethod bindingMethod)
{
    Q_D(QScxmlStateMachine);

    d->m_dataBinding = bindingMethod;
}

/*!
 * \return the binding method used by the state machine.
 */
QScxmlStateMachine::BindingMethod QScxmlStateMachine::dataBinding() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_dataBinding;
}

/*!
 * \internal This is used internally in order to execute the executable content.
 *
 * \return Returns the data tables used by the state-machine.
 */
QScxmlTableData *QScxmlStateMachine::tableData() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_tableData;
}

/*!
 * \internal This is used when generating C++ from an SCXML file. The class implementing the
 * state-machine will use this method to pass in the table data (which is also generated).
 *
 * \return Returns the data tables used by the state-machine.
 */
void QScxmlStateMachine::setTableData(QScxmlTableData *tableData)
{
    Q_D(QScxmlStateMachine);
    Q_ASSERT(tableData);

    d->m_tableData = tableData;
    if (objectName().isEmpty()) {
        setObjectName(tableData->name());
    }
}

/*!
 * \return Returns the parent state-machine if there is one, otherwise it returns null.
 */
QScxmlStateMachine *QScxmlStateMachine::parentStateMachine() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_parentStateMachine;
}

/*!
 * \brief Sets the \a parent of the state-machine. When set, this implies that the state-machine
 * is started by <invoke> tag.
 */
void QScxmlStateMachine::setParentStateMachine(QScxmlStateMachine *parent)
{
    Q_D(QScxmlStateMachine);
    d->m_parentStateMachine = parent;
}

void QScxmlInternal::WrappedQStateMachine::beginSelectTransitions(QEvent *event)
{
    Q_D(WrappedQStateMachine);

    if (event && event->type() == QScxmlEvent::scxmlEventType) {
        stateMachinePrivate()->m_event = *static_cast<QScxmlEvent *>(event);
        d->stateMachine()->dataModel()->setEvent(stateMachinePrivate()->m_event);

        auto scxmlEvent = static_cast<QScxmlEvent *>(event);
        auto smp = stateMachinePrivate();

        foreach (QScxmlInvokableService *service, smp->invokedServices()) {
            if (scxmlEvent->invokeId() == service->id()) {
                service->finalize();
            }
            if (service->autoforward()) {
                qCDebug(scxmlLog) << this << "auto-forwarding event" << scxmlEvent->name()
                                  << "from" << stateMachine()->name() << "to service" << service->id();
                service->submitEvent(new QScxmlEvent(*scxmlEvent));
            }
        }

        auto e = static_cast<QScxmlEvent *>(event);
        if (smp->m_eventFilter && !smp->m_eventFilter->handle(e, d->stateMachine())) {
            e->makeIgnorable();
            e->clear();
            smp->m_event.clear();
            return;
        }
    } else {
        stateMachinePrivate()->m_event.clear();
        d->stateMachine()->dataModel()->setEvent(stateMachinePrivate()->m_event);
    }
}

void QScxmlInternal::WrappedQStateMachine::beginMicrostep(QEvent *event)
{
    Q_D(WrappedQStateMachine);

    qCDebug(scxmlLog) << d->m_stateMachine
                      << "started microstep from state" << d->m_stateMachine->activeStates()
                      << "with event" << stateMachinePrivate()->m_event.name()
                      << "and event type" << event->type();
}

void QScxmlInternal::WrappedQStateMachine::endMicrostep(QEvent *event)
{
    Q_D(WrappedQStateMachine);
    Q_UNUSED(event);

    qCDebug(scxmlLog) << d->m_stateMachine
                      << "finished microstep in state (" << d->m_stateMachine->activeStates() << ")";
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

void QScxmlInternal::WrappedQStateMachinePrivate::noMicrostep()
{
    qCDebug(scxmlLog) << m_stateMachine
                      << "had no transition, stays in state (" << m_stateMachine->activeStates() << ")";
}

void QScxmlInternal::WrappedQStateMachinePrivate::processedPendingEvents(bool didChange)
{
    qCDebug(scxmlLog) << m_stateMachine << "finishedPendingEvents" << didChange << "in state ("
                      << m_stateMachine->activeStates() << ")";
    emit m_stateMachine->reachedStableState(didChange);
}

void QScxmlInternal::WrappedQStateMachinePrivate::beginMacrostep()
{
}

void QScxmlInternal::WrappedQStateMachinePrivate::endMacrostep(bool didChange)
{
    qCDebug(scxmlLog) << m_stateMachine << "endMacrostep" << didChange
                      << "in state (" << m_stateMachine->activeStates() << ")";

    { // handle <invoke>s
        QVector<QScxmlState*> &sti = stateMachinePrivate()->m_statesToInvoke;
        std::sort(sti.begin(), sti.end(), WrappedQStateMachinePrivate::stateEntryLessThan);
        foreach (QScxmlState *s, sti) {
            auto sp = QScxmlStatePrivate::get(s);
            foreach (QScxmlInvokableService *s, sp->servicesWaitingToStart) {
                s->start();
            }
            sp->servicesWaitingToStart.clear();
        }
        sti.clear();
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void QScxmlInternal::WrappedQStateMachinePrivate::enterStates(
        QEvent *event,
        const QList<QAbstractState*> &exitedStates_sorted,
        const QList<QAbstractState*> &statesToEnter_sorted,
        const QSet<QAbstractState*> &statesForDefaultEntry,
        QHash<QAbstractState *, QVector<QPropertyAssignment> > &propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
        , const QList<QAbstractAnimation*> &selectedAnimations
#  endif
        )
{
    QStateMachinePrivate::enterStates(event, exitedStates_sorted, statesToEnter_sorted,
                                      statesForDefaultEntry, propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
                                      , selectedAnimations
#  endif
                                      );
    foreach (QAbstractState *s, statesToEnter_sorted) {
        if (QScxmlState *qss = qobject_cast<QScxmlState *>(s)) {
            if (!QScxmlStatePrivate::get(qss)->invokableServiceFactories.isEmpty()) {
                if (!stateMachinePrivate()->m_statesToInvoke.contains(qss)) {
                    stateMachinePrivate()->m_statesToInvoke.append(qss);
                }
            }
        }
    }
}

void QScxmlInternal::WrappedQStateMachinePrivate::exitStates(
        QEvent *event,
        const QList<QAbstractState *> &statesToExit_sorted,
        const QHash<QAbstractState*, QVector<QPropertyAssignment> > &assignmentsForEnteredStates)
{
    QStateMachinePrivate::exitStates(event, statesToExit_sorted, assignmentsForEnteredStates);

    auto smp = stateMachinePrivate();
    for (int i = 0; i < smp->m_statesToInvoke.size(); ) {
        if (statesToExit_sorted.contains(smp->m_statesToInvoke.at(i))) {
            smp->m_statesToInvoke.removeAt(i);
        } else {
            ++i;
        }
    }

    foreach (QAbstractState *s, statesToExit_sorted) {
        if (QScxmlState *qss = qobject_cast<QScxmlState *>(s)) {
            auto ssp = QScxmlStatePrivate::get(qss);
            ssp->servicesWaitingToStart.clear();
            QVector<QScxmlInvokableService *> &services = ssp->invokedServices;
            foreach (QScxmlInvokableService *service, services) {
                qCDebug(scxmlLog) << stateMachine() << "schedule service cancellation" << service->id();
                QMetaObject::invokeMethod(q_func(),
                                          "removeAndDestroyService",
                                          Qt::QueuedConnection,
                                          Q_ARG(QScxmlInvokableService *,service));
            }
            services.clear();
        }
    }
}

void QScxmlInternal::WrappedQStateMachinePrivate::exitInterpreter()
{
    Q_Q(WrappedQStateMachine);

    foreach (QAbstractState *s, configuration) {
        QScxmlExecutableContent::ContainerId onExitInstructions = QScxmlExecutableContent::NoInstruction;
        if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(s)) {
            stateMachinePrivate()->m_executionEngine->execute(finalState->doneData(), QVariant());
            onExitInstructions = QScxmlFinalState::Data::get(finalState)->onExitInstructions;
        } else if (QScxmlState *state = qobject_cast<QScxmlState *>(s)) {
            onExitInstructions = QScxmlStatePrivate::get(state)->onExitInstructions;
        }

        if (onExitInstructions != QScxmlExecutableContent::NoInstruction) {
            stateMachinePrivate()->m_executionEngine->execute(onExitInstructions);
        }

        if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(s)) {
            if (finalState->parent() == q) {
                auto psm = m_stateMachine->parentStateMachine();
                if (psm) {
                    auto done = new QScxmlEvent;
                    done->setName(QByteArray("done.invoke.") + m_stateMachine->sessionId().toUtf8());
                    done->setInvokeId(m_stateMachine->sessionId());
                    qCDebug(scxmlLog) << "submitting event" << done->name() << "to" << psm->name();
                    psm->submitEvent(done);
                }
            }
        }
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)

void QScxmlInternal::WrappedQStateMachinePrivate::emitStateFinished(QState *forState, QFinalState *guiltyState)
{
    Q_Q(WrappedQStateMachine);

    if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(guiltyState)) {
        if (!q->isRunning())
            return;
        stateMachinePrivate()->m_executionEngine->execute(finalState->doneData(), forState->objectName());
    }

    QStateMachinePrivate::emitStateFinished(forState, guiltyState);
}

void QScxmlInternal::WrappedQStateMachinePrivate::startupHook()
{
    Q_Q(WrappedQStateMachine);

    q->submitQueuedEvents();
}

#endif // QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

int QScxmlInternal::WrappedQStateMachinePrivate::eventIdForDelayedEvent(const QByteArray &scxmlEventId)
{
    QMutexLocker locker(&delayedEventsMutex);

    QHash<int, DelayedEvent>::const_iterator it;
    for (it = delayedEvents.constBegin(); it != delayedEvents.constEnd(); ++it) {
        if (QScxmlEvent *e = dynamic_cast<QScxmlEvent *>(it->event)) {
            if (e->sendId() == scxmlEventId) {
                return it.key();
            }
        }
    }

    return -1;
}

QStringList QScxmlStateMachine::activeStates(bool compress)
{
    Q_D(QScxmlStateMachine);

    QSet<QAbstractState *> config = QStateMachinePrivate::get(d->m_qStateMachine)->configuration;
    if (compress)
        foreach (const QAbstractState *s, config)
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

bool QScxmlStateMachine::isActive(const QString &scxmlStateName) const
{
    Q_D(const QScxmlStateMachine);
    QSet<QAbstractState *> config = QStateMachinePrivate::get(d->m_qStateMachine)->configuration;
    foreach (QAbstractState *s, config) {
        if (s->objectName() == scxmlStateName) {
            return true;
        }
    }
    return false;
}

bool QScxmlStateMachine::hasState(const QString &scxmlStateName) const
{
    Q_D(const QScxmlStateMachine);
    return findState(scxmlStateName, d->m_qStateMachine) != Q_NULLPTR;
}

QMetaObject::Connection QScxmlStateMachine::connect(const QString &scxmlStateName, const char *signal,
                                            const QObject *receiver, const char *method,
                                            Qt::ConnectionType type)
{
    Q_D(QScxmlStateMachine);
    QScxmlState *state = findState(scxmlStateName, d->m_qStateMachine);
    return QObject::connect(state, signal, receiver, method, type);
}

QScxmlEventFilter *QScxmlStateMachine::scxmlEventFilter() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_eventFilter;
}

void QScxmlStateMachine::setScxmlEventFilter(QScxmlEventFilter *newFilter)
{
    Q_D(QScxmlStateMachine);
    d->m_eventFilter = newFilter;
}

static bool loopOnSubStates(QState *startState,
                            std::function<bool(QState *)> enteringState = Q_NULLPTR,
                            std::function<void(QState *)> exitingState = Q_NULLPTR,
                            std::function<void(QAbstractState *)> inAbstractState = Q_NULLPTR)
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

bool QScxmlStateMachine::init(const QVariantMap &initialDataValues)
{
    Q_D(QScxmlStateMachine);

    dataModel()->setup(initialDataValues);
    d->executeInitialSetup();

    bool success = true;
    loopOnSubStates(d->m_qStateMachine, std::function<bool(QState *)>(), [&success](QState *state) {
        if (QScxmlState *s = qobject_cast<QScxmlState *>(state)) {
            if (!s->init()) {
                success = false;
            }
        }
        if (QScxmlFinalState *s = qobject_cast<QScxmlFinalState *>(state)) {
            if (!s->init()) {
                success = false;
            }
        }
        foreach (QObject *child, state->children()) {
            if (QScxmlTransition *transition = qobject_cast<QScxmlTransition *>(child)) {
                if (!transition->init()) {
                    success = false;
                }
            }
        }
    });
    foreach (QObject *child, d->m_qStateMachine->children()) {
        if (QScxmlTransition *transition = qobject_cast<QScxmlTransition *>(child)) {
            if (!transition->init()) {
                success = false;
            }
        }
    }
    return success;
}

bool QScxmlStateMachine::isRunning() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_qStateMachine->isRunning();
}

QString QScxmlStateMachine::name() const
{
    return tableData()->name();
}

/*!
 * \brief Submits an error event to the external event queue of this state-machine.
 *
 * \param type The error message type, e.g. "error.execution". The type has to start with "error.".
 * \param msg A string describing the nature of the error. This is passed to the event as the
 *            errorMessage
 * \param sendid The sendid of the message causing the error, if it has one.
 */
void QScxmlStateMachine::submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid)
{
    qCDebug(scxmlLog) << this << "had error" << type << ":" << msg;
    if (!type.startsWith("error."))
        qCWarning(scxmlLog) << this << "Message type of error message does not start with 'error.'!";
    submitEvent(QScxmlEventBuilder::errorEvent(this, type, msg, sendid));
}

/*!
 * \brief Submits an event for routing.
 *
 * Depending on the origin of the event, it gets submitted to the parent state-machine (if there is
 * one), to a child state-machine, or to the current state-machine.
 *
 * \param event The event to be routed.
 */
void QScxmlStateMachine::routeEvent(QScxmlEvent *event)
{
    Q_D(QScxmlStateMachine);

    if (!event)
        return;

    QString origin = event->origin();
    if (origin == QStringLiteral("#_parent")) {
        if (auto psm = parentStateMachine()) {
            qCDebug(scxmlLog) << this << "routing event" << event->name() << "from" << name() << "to parent" << psm->name();
            psm->submitEvent(event);
        } else {
            qCDebug(scxmlLog) << this << "is not invoked, so it cannot route a message to #_parent";
            delete event;
        }
    } else if (origin.startsWith(QStringLiteral("#_")) && origin != QStringLiteral("#_internal")) {
        // route to children
        auto originId = origin.midRef(2);
        foreach (QScxmlInvokableService *service, d->m_invokedServices) {
            if (service->id() == originId) {
                qCDebug(scxmlLog) << this << "routing event" << event->name()
                                  << "from" << name()
                                  << "to parent" << service->id();
                service->submitEvent(new QScxmlEvent(*event));
            }
        }
        delete event;
    } else {
        submitEvent(event);
    }
}

void QScxmlStateMachine::submitEvent(QScxmlEvent *event)
{
    Q_D(QScxmlStateMachine);

    if (!event)
        return;

    if (event->delay() > 0) {
        qCDebug(scxmlLog) << this << ": submitting event" << event->name()
                          << "with delay" << event->delay() << "ms"
                          << "and sendid" << event->sendId();

        Q_ASSERT(event->eventType() == QScxmlEvent::ExternalEvent);
        int id = d->m_qStateMachine->postDelayedEvent(event, event->delay());

        qCDebug(scxmlLog) << this << ": delayed event" << event->name() << "(" << event << ") got id:" << id;
    } else {
        QStateMachine::EventPriority priority =
                event->eventType() == QScxmlEvent::ExternalEvent ? QStateMachine::NormalPriority
                                                             : QStateMachine::HighPriority;

        if (d->m_qStateMachine->isRunning()) {
            qCDebug(scxmlLog) << this << "posting event" << event->name();
            d->m_qStateMachine->postEvent(event, priority);
        } else {
            qCDebug(scxmlLog) << this << "queueing event" << event->name();
            d->m_qStateMachine->queueEvent(event, priority);
        }
    }
}

/*!
 * \brief Utility method to create and submit an external event with the givent \a eventName as
 *        the name.
 */
void QScxmlStateMachine::submitEvent(const QByteArray &eventName)
{
    QScxmlEvent *e = new QScxmlEvent;
    e->setName(eventName);
    e->setEventType(QScxmlEvent::ExternalEvent);
    submitEvent(e);
}

/*!
 * \brief Utility method to create and submit an external event with the givent \a eventName as
 *        the name and \a data as the payload data.
 */
void QScxmlStateMachine::submitEvent(const QByteArray &event, const QVariant &data)
{
    QVariant incomingData = data;
    if (incomingData.canConvert<QJSValue>()) {
        incomingData = incomingData.value<QJSValue>().toVariant();
    }

    QScxmlEvent *e = new QScxmlEvent;
    e->setName(event);
    e->setEventType(QScxmlEvent::ExternalEvent);
    e->setData(incomingData);
    submitEvent(e);
}

/*!
 * \brief Cancels a delayed event with the given \a sendid.
 */
void QScxmlStateMachine::cancelDelayedEvent(const QByteArray &sendid)
{
    Q_D(QScxmlStateMachine);

    int id = d->m_qStateMachine->eventIdForDelayedEvent(sendid);

    qCDebug(scxmlLog) << this << "canceling event" << sendid << "with id" << id;

    if (id != -1)
        d->m_qStateMachine->cancelDelayedEvent(id);
}

void QScxmlInternal::WrappedQStateMachine::queueEvent(QScxmlEvent *event, EventPriority priority)
{
    Q_D(WrappedQStateMachine);

    if (!d->m_queuedEvents)
        d->m_queuedEvents = new QVector<WrappedQStateMachinePrivate::QueuedEvent>();
    d->m_queuedEvents->append(WrappedQStateMachinePrivate::QueuedEvent(event, priority));
}

void QScxmlInternal::WrappedQStateMachine::submitQueuedEvents()
{
    Q_D(WrappedQStateMachine);

    qCDebug(scxmlLog) << d->m_stateMachine << ": submitting queued events";

    if (d->m_queuedEvents) {
        foreach (const WrappedQStateMachinePrivate::QueuedEvent &e, *d->m_queuedEvents)
            postEvent(e.event, e.priority);
        delete d->m_queuedEvents;
        d->m_queuedEvents = Q_NULLPTR;
    }
}

int QScxmlInternal::WrappedQStateMachine::eventIdForDelayedEvent(const QByteArray &scxmlEventId)
{
    Q_D(WrappedQStateMachine);
    return d->eventIdForDelayedEvent(scxmlEventId);
}

void QScxmlInternal::WrappedQStateMachine::removeAndDestroyService(QScxmlInvokableService *service)
{
    Q_D(WrappedQStateMachine);
    qCDebug(scxmlLog) << stateMachine() << "canceling service" << service->id();
    if (d->stateMachinePrivate()->removeService(service)) {
        delete service;
    }
}

bool QScxmlStateMachine::isDispatchableTarget(const QString &target) const
{
    Q_D(const QScxmlStateMachine);

    if (isInvoked() && target == QStringLiteral("#_parent"))
        return true; // parent state machine, if we're <invoke>d.
    if (target == QStringLiteral("#_internal")
            || target == QStringLiteral("#_scxml_%1").arg(sessionId()))
        return true; // that's the current state machine

    if (target.startsWith(QStringLiteral("#_"))) {
        QStringRef targetId = target.midRef(2);
        foreach (QScxmlInvokableService *service, d->m_invokedServices) {
            if (service->id() == targetId) {
                return true;
            }
        }
    }

    return false;
}

/*!
  \fn QScxmlStateMachine::log(const QString &label, const QString &msg)
  \since 5.6

  This signal is emitted where a <log> tag is used in the Scxml.

  \param label The value of the label attribute of the <log> tag.
  \param msg The value of the evaluated expr attribute of the <log> tag. If there was no expr
         attribute, a null string will be returned.
*/

/*!
  \fn QScxmlStateMachine::reachedStableState(bool didChange)
  \since 5.6

  This signal is emitted when the event queue is empty at the end of a macro step, or when a final
  state is reached.
*/

void QScxmlStateMachine::start()
{
    Q_D(QScxmlStateMachine);

    d->m_qStateMachine->start();
}

void QScxmlStateMachine::setService(const QString &id, QScxmlInvokableService *service)
{
    Q_UNUSED(id);
    Q_UNUSED(service);
}

QT_END_NAMESPACE
