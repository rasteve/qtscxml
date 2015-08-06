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

#include "scxmlqstates.h"
#include "scxmlstatetable_p.h"

namespace Scxml {

class ScxmlBaseTransition::Data
{
public:
    QList<QByteArray> eventSelector;
};

ScxmlBaseTransition::ScxmlBaseTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : QAbstractTransition(sourceState)
    , d(new Data)
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
    if (Internal::MyQStateMachine *t = qobject_cast<Internal::MyQStateMachine *>(parent()))
        return t->stateTable();
    if (QState *s = sourceState())
        return qobject_cast<Internal::MyQStateMachine *>(s->machine())->stateTable();
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
    QByteArray eventName = StateTablePrivate::get(stateTable)->m_event.name();
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
    return selected;
}

bool ScxmlBaseTransition::clear()
{
    return true;
}

bool ScxmlBaseTransition::init()
{
    return true;
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
    Data()
        : conditionalExp(NoEvaluator)
        , instructionsOnTransition(ExecutableContent::NoInstruction)
    {}

    EvaluatorId conditionalExp;
    ExecutableContent::ContainerId instructionsOnTransition;
};

ScxmlTransition::ScxmlTransition(QState *sourceState, const QList<QByteArray> &eventSelector)
    : ScxmlBaseTransition(sourceState, filterEmpty(eventSelector))
    , d(new Data)
{}

ScxmlTransition::ScxmlTransition(const QList<QByteArray> &eventSelector)
    : ScxmlBaseTransition(Q_NULLPTR, filterEmpty(eventSelector))
    , d(new Data)
{}

ScxmlTransition::~ScxmlTransition()
{
    delete d;
}

bool ScxmlTransition::eventTest(QEvent *event)
{
#ifdef DUMP_EVENT
    if (auto edm = table()->dataModel()->asEcmaScriptDataModel()) qCDebug(scxmlLog) << qPrintable(edm->engine()->evaluate(QLatin1String("JSON.stringify(_event)")).toString());
#endif

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
    if (Internal::MyQStateMachine *t = qobject_cast<Internal::MyQStateMachine *>(sourceState()))
        return t->stateTable();
    return qobject_cast<Internal::MyQStateMachine *>(machine())->stateTable();
}

void ScxmlTransition::setInstructionsOnTransition(ExecutableContent::ContainerId instructions)
{
    d->instructionsOnTransition = instructions;
}

void ScxmlTransition::setConditionalExpression(EvaluatorId evaluator)
{
    d->conditionalExp = evaluator;
}

class ScxmlStatePrivate: public QStatePrivate
{
    Q_DECLARE_PUBLIC(ScxmlState)

public:
    ScxmlStatePrivate()
        : initInstructions(ExecutableContent::NoInstruction)
        , onEntryInstructions(ExecutableContent::NoInstruction)
        , onExitInstructions(ExecutableContent::NoInstruction)
    {}

    ExecutableContent::ContainerId initInstructions;
    ExecutableContent::ContainerId onEntryInstructions;
    ExecutableContent::ContainerId onExitInstructions;
};

ScxmlState::ScxmlState(QState *parent)
    : QState(*new ScxmlStatePrivate, parent)
{}

StateTable *ScxmlState::table() const {
    return qobject_cast<Internal::MyQStateMachine *>(machine())->stateTable();
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
    Q_D(ScxmlState);

    d->initInstructions = instructions;
}

void ScxmlState::setOnEntryInstructions(ExecutableContent::ContainerId instructions)
{
    Q_D(ScxmlState);

    d->onEntryInstructions = instructions;
}

void ScxmlState::setOnExitInstructions(ExecutableContent::ContainerId instructions)
{
    Q_D(ScxmlState);

    d->onExitInstructions = instructions;
}

ScxmlState::ScxmlState(Scxml::ScxmlStatePrivate &dd, QState *parent)
    : QState(dd, parent)
{}

void ScxmlState::onEntry(QEvent *event)
{
    Q_D(ScxmlState);

    if (d->initInstructions != ExecutableContent::NoInstruction) {
        table()->executionEngine()->execute(d->initInstructions);
        d->initInstructions = ExecutableContent::NoInstruction;
    }
    QState::onEntry(event);
    table()->executionEngine()->execute(d->onEntryInstructions);
    emit didEnter();
}

void ScxmlState::onExit(QEvent *event)
{
    Q_D(ScxmlState);

    emit willExit();
    QState::onExit(event);
    table()->executionEngine()->execute(d->onExitInstructions);
}

class ScxmlFinalState::Data
{
public:
    Data()
        : doneData(ExecutableContent::NoInstruction)
        , onEntryInstructions(ExecutableContent::NoInstruction)
        , onExitInstructions(ExecutableContent::NoInstruction)
    {}

    ExecutableContent::ContainerId doneData;
    ExecutableContent::ContainerId onEntryInstructions;
    ExecutableContent::ContainerId onExitInstructions;
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
    return qobject_cast<Internal::MyQStateMachine *>(machine())->stateTable();
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

} // Scxml namespace
