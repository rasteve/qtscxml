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

#include <QtScxml/qscxmlstatemachine.h>
#include <QtScxml/qscxmlinvokableservice.h>

#include <QAbstractTransition>
#include <QFinalState>
#include <QState>

QT_BEGIN_NAMESPACE

template<class T>
class QScxmlInvokeScxmlFactory: public QScxmlInvokableScxmlServiceFactory
{
public:
    QScxmlInvokeScxmlFactory(QScxmlExecutableContent::StringId invokeLocation,
                             QScxmlExecutableContent::StringId id,
                             QScxmlExecutableContent::StringId idPrefix,
                             QScxmlExecutableContent::StringId idlocation,
                             const QVector<QScxmlExecutableContent::StringId> &namelist,
                             bool doAutoforward,
                             const QVector<Param> &params,
                             QScxmlExecutableContent::ContainerId finalize)
        : QScxmlInvokableScxmlServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist,
                                             doAutoforward, params, finalize)
    {}

    QScxmlInvokableService *invoke(QScxmlStateMachine *parent) Q_DECL_OVERRIDE
    {
        return finishInvoke(new T, parent);
    }
};

class QScxmlStatePrivate;
class Q_SCXML_EXPORT QScxmlState: public QState
{
    Q_OBJECT

public:
    QScxmlState(QState *parent = Q_NULLPTR);
    QScxmlState(QScxmlStateMachine *parent);
    ~QScxmlState();

    void setAsInitialStateFor(QScxmlState *state);
    void setAsInitialStateFor(QScxmlStateMachine *stateMachine);

    QScxmlStateMachine *stateMachine() const;
    virtual bool init();
    QString stateLocation() const;

    void setInitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setInvokableServiceFactories(const QVector<QScxmlInvokableServiceFactory *>& factories);

Q_SIGNALS:
    void didEnter();
    void willExit();

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    friend class QScxmlStatePrivate;
    QScxmlStatePrivate *d;
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
    QScxmlFinalState(QScxmlStateMachine *parent);
    ~QScxmlFinalState();

    void setAsInitialStateFor(QScxmlState *state);
    void setAsInitialStateFor(QScxmlStateMachine *stateMachine);

    QScxmlStateMachine *stateMachine() const;
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

    QScxmlStateMachine *stateMachine() const;
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
    void addTransitionTo(QScxmlStateMachine *stateMachine);

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    QScxmlStateMachine *stateMachine() const;

    void setInstructionsOnTransition(QScxmlExecutableContent::ContainerId instructions);
    void setConditionalExpression(QScxmlExecutableContent::EvaluatorId evaluator);

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

QT_END_NAMESPACE

#endif // SCXMLQSTATES_H
