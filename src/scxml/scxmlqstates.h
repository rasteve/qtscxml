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
    InvokeScxmlFactory(Scxml::QScxmlExecutableContent::StringId invokeLocation,
                       Scxml::QScxmlExecutableContent::StringId id,
                       Scxml::QScxmlExecutableContent::StringId idPrefix,
                       Scxml::QScxmlExecutableContent::StringId idlocation,
                       const QVector<Scxml::QScxmlExecutableContent::StringId> &namelist,
                       bool doAutoforward,
                       const QVector<Param> &params,
                       QScxmlExecutableContent::ContainerId finalize)
        : InvokableScxmlServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist,
                                       doAutoforward, params, finalize)
    {}

    Scxml::ScxmlInvokableService *invoke(StateMachine *parent) Q_DECL_OVERRIDE
    {
        return finishInvoke(new T, parent);
    }
};

class Q_SCXML_EXPORT QScxmlState: public QState
{
    Q_OBJECT

public:
    class Data;

    QScxmlState(QState *parent = Q_NULLPTR);
    QScxmlState(StateMachine *parent);
    ~QScxmlState();

    void setAsInitialStateFor(QScxmlState *state);
    void setAsInitialStateFor(StateMachine *stateMachine);

    StateMachine *stateMachine() const;
    virtual bool init();
    QString stateLocation() const;

    void setInitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setInvokableServiceFactories(const QVector<ScxmlInvokableServiceFactory *>& factories);

Q_SIGNALS:
    void didEnter();
    void willExit();

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class Q_SCXML_EXPORT QScxmlInitialState: public QScxmlState
{
    Q_OBJECT
public:
    QScxmlInitialState(QState *theParent);
};

class Q_SCXML_EXPORT QScxmlFinalState: public QFinalState
{
    Q_OBJECT
public:
    class Data;

    QScxmlFinalState(QState *parent = Q_NULLPTR);
    QScxmlFinalState(StateMachine *parent);
    ~QScxmlFinalState();

    void setAsInitialStateFor(QScxmlState *state);
    void setAsInitialStateFor(StateMachine *stateMachine);

    StateMachine *stateMachine() const;
    virtual bool init();

    QScxmlExecutableContent::ContainerId doneData() const;
    void setDoneData(QScxmlExecutableContent::ContainerId doneData);

    void setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions);

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class Q_SCXML_EXPORT QScxmlBaseTransition: public QAbstractTransition
{
    Q_OBJECT
    class Data;

public:
    QScxmlBaseTransition(QState * sourceState = Q_NULLPTR,
                         const QList<QByteArray> &eventSelector = QList<QByteArray>());
    QScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                         const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ~QScxmlBaseTransition();

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

class Q_SCXML_EXPORT QScxmlTransition: public QScxmlBaseTransition
{
    Q_OBJECT
    class Data;

public:
    QScxmlTransition(QState * sourceState = Q_NULLPTR,
                     const QList<QByteArray> &eventSelector = QList<QByteArray>());
    QScxmlTransition(const QList<QByteArray> &eventSelector);
    ~QScxmlTransition();

    void addTransitionTo(QScxmlState *state);
    void addTransitionTo(StateMachine *stateMachine);

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    StateMachine *stateMachine() const;

    void setInstructionsOnTransition(QScxmlExecutableContent::ContainerId instructions);
    void setConditionalExpression(EvaluatorId evaluator);

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

} // Scxml namespace

QT_END_NAMESPACE

#endif // SCXMLQSTATES_H
