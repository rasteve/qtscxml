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
    QString stateLocation() const;

    void setInitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnEntryInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setOnExitInstructions(QScxmlExecutableContent::ContainerId instructions);
    void setInvokableServiceFactories(const QVector<QScxmlInvokableServiceFactory *>& factories);

Q_SIGNALS:
    void didEnter(); // TODO: REMOVE!
    void willExit(); // TODO: REMOVE!

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    friend class QScxmlStatePrivate;
    QScxmlStatePrivate *d;
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
                         const QStringList &eventSelector = QStringList());
    QScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                         const QStringList &eventSelector = QStringList());
    ~QScxmlBaseTransition();

    QScxmlStateMachine *stateMachine() const;
    QString transitionLocation() const;

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;

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
                     const QStringList &eventSelector = QStringList());
    QScxmlTransition(const QStringList &eventSelector);
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
