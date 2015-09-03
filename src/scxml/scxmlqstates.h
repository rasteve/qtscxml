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

#ifndef SCXMLQSTATES_H
#define SCXMLQSTATES_H

#include <QtScxml/scxmlstatemachine.h>

QT_BEGIN_NAMESPACE

namespace Scxml {

template<class T>
class InvokeScxmlFactory: public Scxml::InvokableScxmlServiceFactory
{
public:
    InvokeScxmlFactory(Scxml::ExecutableContent::StringId invokeLocation,
                       Scxml::ExecutableContent::StringId id,
                       Scxml::ExecutableContent::StringId idPrefix,
                       Scxml::ExecutableContent::StringId idlocation,
                       const QVector<Scxml::ExecutableContent::StringId> &namelist,
                       bool autoforward,
                       const QVector<Param> &params)
        : InvokableScxmlServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist, autoforward, params)
    {}

    Scxml::ScxmlInvokableService *invoke(StateMachine *parent) Q_DECL_OVERRIDE
    {
        return finishInvoke(new T, parent);
    }
};

class Q_SCXML_EXPORT ScxmlBaseTransition : public QAbstractTransition
{
    Q_OBJECT
    class Data;

public:
    ScxmlBaseTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                        const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ~ScxmlBaseTransition();

    StateMachine *stateMachine() const;
    QString transitionLocation() const;

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    virtual bool clear();
    virtual bool init();

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class Q_SCXML_EXPORT ScxmlTransition : public ScxmlBaseTransition
{
    Q_OBJECT
    class Data;

public:
    ScxmlTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ScxmlTransition(const QList<QByteArray> &eventSelector);
    ~ScxmlTransition();

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    StateMachine *stateMachine() const;

    void setInstructionsOnTransition(ExecutableContent::ContainerId instructions);
    void setConditionalExpression(EvaluatorId evaluator);

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class Q_SCXML_EXPORT ScxmlState: public QState
{
    Q_OBJECT

public:
    ScxmlState(QState *parent = 0);
    ~ScxmlState();

    StateMachine *stateMachine() const;
    virtual bool init();
    QString stateLocation() const;

    void setInitInstructions(ExecutableContent::ContainerId instructions);
    void setOnEntryInstructions(ExecutableContent::ContainerId instructions);
    void setOnExitInstructions(ExecutableContent::ContainerId instructions);
    void setInvokableServiceFactories(const QVector<ScxmlInvokableServiceFactory *>& factories);

Q_SIGNALS:
    void didEnter();
    void willExit();

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    class Data;
    Data *d;
};

class Q_SCXML_EXPORT ScxmlInitialState: public ScxmlState
{
    Q_OBJECT
public:
    ScxmlInitialState(QState *theParent): ScxmlState(theParent) { }
};

class Q_SCXML_EXPORT ScxmlFinalState: public QFinalState
{
    Q_OBJECT
    class Data;
public:
    ScxmlFinalState(QState *parent = 0);
    ~ScxmlFinalState();

    StateMachine *stateMachine() const;
    virtual bool init();

    ExecutableContent::ContainerId doneData() const;
    void setDoneData(ExecutableContent::ContainerId doneData);

    void setOnEntryInstructions(ExecutableContent::ContainerId instructions);
    void setOnExitInstructions(ExecutableContent::ContainerId instructions);

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

} // Scxml namespace

QT_END_NAMESPACE

#endif // SCXMLQSTATES_H
