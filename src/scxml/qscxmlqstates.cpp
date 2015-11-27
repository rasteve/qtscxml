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

#include "qscxmlqstates_p.h"
#include "qscxmlstatemachine_p.h"

#undef DUMP_EVENT
#ifdef DUMP_EVENT
#include <QJSEngine>
#include "qscxmlecmascriptdatamodel.h"
#endif

QT_BEGIN_NAMESPACE

class QScxmlBaseTransition::Data
{
public:
    QList<QByteArray> eventSelector;
};

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

QScxmlState::QScxmlState(QState *parent)
    : QState(parent)
    , d(new QScxmlStatePrivate(this))
{}

QScxmlState::QScxmlState(QScxmlStateMachine *parent)
    : QState(QScxmlStateMachinePrivate::get(parent)->m_qStateMachine)
    , d(new QScxmlStatePrivate(this))
{}

QScxmlState::~QScxmlState()
{
    delete d;
}

void QScxmlState::setAsInitialStateFor(QScxmlState *state)
{
    state->setInitialState(this);
}

void QScxmlState::setAsInitialStateFor(QScxmlStateMachine *stateMachine)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_qStateMachine->setInitialState(this);
}

QScxmlStateMachine *QScxmlState::stateMachine() const {
    return qobject_cast<QScxmlInternal::WrappedQStateMachine *>(machine())->stateMachine();
}

QString QScxmlState::stateLocation() const
{
    return QStringLiteral("State %1").arg(objectName());
}

void QScxmlState::setInitInstructions(QScxmlExecutableContent::ContainerId instructions)
{
    d->initInstructions = instructions;
}

void QScxmlState::setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions)
{
    d->onEntryInstructions = instructions;
}

void QScxmlState::setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions)
{
    d->onExitInstructions = instructions;
}

void QScxmlState::setInvokableServiceFactories(const QVector<QScxmlInvokableServiceFactory *> &factories)
{
    d->invokableServiceFactories = factories;
}

void QScxmlState::onEntry(QEvent *event)
{
    auto sp = QScxmlStateMachinePrivate::get(stateMachine());
    if (d->initInstructions != QScxmlExecutableContent::NoInstruction) {
        sp->m_executionEngine->execute(d->initInstructions);
        d->initInstructions = QScxmlExecutableContent::NoInstruction;
    }
    QState::onEntry(event);
    auto sm = stateMachine();
    QScxmlStateMachinePrivate::get(sm)->m_executionEngine->execute(d->onEntryInstructions);
    foreach (QScxmlInvokableServiceFactory *f, d->invokableServiceFactories) {
        if (auto service = f->invoke(stateMachine())) {
            d->invokedServices.append(service);
            d->servicesWaitingToStart.append(service);
            sp->addService(service);
        }
    }
    emit didEnter();
}

void QScxmlState::onExit(QEvent *event)
{
    emit willExit();
    auto sm = stateMachine();
    QScxmlStateMachinePrivate::get(sm)->m_executionEngine->execute(d->onExitInstructions);
    QState::onExit(event);
}

QScxmlInitialState::QScxmlInitialState(QState *theParent)
    : QScxmlState(theParent)
{}

QScxmlFinalState::QScxmlFinalState(QState *parent)
    : QFinalState(parent)
    , d(new Data)
{}

QScxmlFinalState::QScxmlFinalState(QScxmlStateMachine *parent)
    : QFinalState(QScxmlStateMachinePrivate::get(parent)->m_qStateMachine)
    , d(new Data)
{}

QScxmlFinalState::~QScxmlFinalState()
{
    delete d;
}

void QScxmlFinalState::setAsInitialStateFor(QScxmlState *state)
{
    state->setInitialState(this);
}

void QScxmlFinalState::setAsInitialStateFor(QScxmlStateMachine *stateMachine)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_qStateMachine->setInitialState(this);
}

QScxmlStateMachine *QScxmlFinalState::stateMachine() const {
    return qobject_cast<QScxmlInternal::WrappedQStateMachine *>(machine())->stateMachine();
}

QScxmlExecutableContent::ContainerId QScxmlFinalState::doneData() const
{
    return d->doneData;
}

void QScxmlFinalState::setDoneData(QScxmlExecutableContent::ContainerId doneData)
{
    d->doneData = doneData;
}

void QScxmlFinalState::setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions)
{
    d->onEntryInstructions = instructions;
}

void QScxmlFinalState::setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions)
{
    d->onExitInstructions = instructions;
}

void QScxmlFinalState::onEntry(QEvent *event)
{
    QFinalState::onEntry(event);
    auto smp = QScxmlStateMachinePrivate::get(stateMachine());
    smp->m_executionEngine->execute(d->onEntryInstructions);
}

void QScxmlFinalState::onExit(QEvent *event)
{
    QFinalState::onExit(event);
    QScxmlStateMachinePrivate::get(stateMachine())->m_executionEngine->execute(d->onExitInstructions);
}

QScxmlBaseTransition::QScxmlBaseTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : QAbstractTransition(sourceState)
    , d(new Data)
{
    d->eventSelector = eventSelector;
}

QScxmlBaseTransition::QScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                                         const QList<QByteArray> &eventSelector)
    : QAbstractTransition(dd, parent)
    , d(new Data)
{
    d->eventSelector = eventSelector;
}

QScxmlBaseTransition::~QScxmlBaseTransition()
{
    delete d;
}

QScxmlStateMachine *QScxmlBaseTransition::stateMachine() const {
    if (QScxmlInternal::WrappedQStateMachine *t = qobject_cast<QScxmlInternal::WrappedQStateMachine *>(parent()))
        return t->stateMachine();
    if (QState *s = sourceState())
        return qobject_cast<QScxmlInternal::WrappedQStateMachine *>(s->machine())->stateMachine();
    qCWarning(scxmlLog) << "could not find Scxml::StateMachine in " << transitionLocation();
    return 0;
}

QString QScxmlBaseTransition::transitionLocation() const {
    if (QState *state = sourceState()) {
        QString stateName = state->objectName();
        int transitionIndex = state->transitions().indexOf(const_cast<QScxmlBaseTransition *>(this));
        return QStringLiteral("transition #%1 in state %2").arg(transitionIndex).arg(stateName);
    }
    return QStringLiteral("unbound transition @%1").arg(reinterpret_cast<quintptr>(this));
}

bool QScxmlBaseTransition::eventTest(QEvent *event)
{
    if (d->eventSelector.isEmpty())
        return true;
    if (event->type() == QEvent::None)
        return false;
    Q_ASSERT(stateMachine());
    QByteArray eventName = QScxmlStateMachinePrivate::get(stateMachine())->m_event.name();
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
                break;
            }
        }
    }
    return selected;
}

void QScxmlBaseTransition::onTransition(QEvent *event)
{
    Q_UNUSED(event);
}

class QScxmlTransition::Data
{
public:
    Data()
        : conditionalExp(QScxmlExecutableContent::NoEvaluator)
        , instructionsOnTransition(QScxmlExecutableContent::NoInstruction)
    {}

    QScxmlExecutableContent::EvaluatorId conditionalExp;
    QScxmlExecutableContent::ContainerId instructionsOnTransition;
};

QScxmlTransition::QScxmlTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : QScxmlBaseTransition(sourceState, filterEmpty(eventSelector))
    , d(new Data)
{}

QScxmlTransition::QScxmlTransition(const QList<QByteArray> &eventSelector)
    : QScxmlBaseTransition(Q_NULLPTR, filterEmpty(eventSelector))
    , d(new Data)
{}

QScxmlTransition::~QScxmlTransition()
{
    delete d;
}

void QScxmlTransition::addTransitionTo(QScxmlState *state)
{
    state->addTransition(this);
}

void QScxmlTransition::addTransitionTo(QScxmlStateMachine *stateMachine)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_qStateMachine->addTransition(this);
}

bool QScxmlTransition::eventTest(QEvent *event)
{
#ifdef DUMP_EVENT
    if (auto edm = dynamic_cast<QScxmlEcmaScriptDataModel *>(stateMachine()->dataModel()))
        qCDebug(scxmlLog) << qPrintable(edm->engine()->evaluate(QLatin1String("JSON.stringify(_event)")).toString());
#endif

    if (QScxmlBaseTransition::eventTest(event)) {
        bool ok = true;
        if (d->conditionalExp != QScxmlExecutableContent::NoEvaluator)
            return stateMachine()->dataModel()->evaluateToBool(d->conditionalExp, &ok) && ok;
        return true;
    }

    return false;
}

void QScxmlTransition::onTransition(QEvent *)
{
    QScxmlStateMachinePrivate::get(stateMachine())->m_executionEngine->execute(d->instructionsOnTransition);
}

QScxmlStateMachine *QScxmlTransition::stateMachine() const {
    // work around a bug in QStateMachine
    if (QScxmlInternal::WrappedQStateMachine *t = qobject_cast<QScxmlInternal::WrappedQStateMachine *>(sourceState()))
        return t->stateMachine();
    return qobject_cast<QScxmlInternal::WrappedQStateMachine *>(machine())->stateMachine();
}

void QScxmlTransition::setInstructionsOnTransition(QScxmlExecutableContent::ContainerId instructions)
{
    d->instructionsOnTransition = instructions;
}

void QScxmlTransition::setConditionalExpression(QScxmlExecutableContent::EvaluatorId evaluator)
{
    d->conditionalExp = evaluator;
}

QT_END_NAMESPACE
