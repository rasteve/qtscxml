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

#ifndef SCXMLSTATEMACHINE_P_H
#define SCXMLSTATEMACHINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtScxml/private/qscxmlexecutablecontent_p.h>
#include <QtScxml/qscxmlstatemachine.h>

#include <QStateMachine>
#include <QtCore/private/qstatemachine_p.h>

QT_BEGIN_NAMESPACE

namespace QScxmlInternal {
class WrappedQStateMachinePrivate;
class WrappedQStateMachine: public QStateMachine
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WrappedQStateMachine)

public:
    WrappedQStateMachine(QScxmlStateMachine *parent);
    WrappedQStateMachine(WrappedQStateMachinePrivate &dd, QScxmlStateMachine *parent);

    QScxmlStateMachine *stateMachine() const;

    void queueEvent(QScxmlEvent *event, QStateMachine::EventPriority priority);
    void submitQueuedEvents();
    int eventIdForDelayedEvent(const QString &sendId);

    Q_INVOKABLE void removeAndDestroyService(QScxmlInvokableService *service);

protected:
    void beginSelectTransitions(QEvent *event) Q_DECL_OVERRIDE;
    void beginMicrostep(QEvent *event) Q_DECL_OVERRIDE;
    void endMicrostep(QEvent *event) Q_DECL_OVERRIDE;

private:
    QScxmlStateMachinePrivate *stateMachinePrivate();
};
} // Internal namespace

class QScxmlInvokableService;
class QScxmlState;
class QScxmlStateMachinePrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QScxmlStateMachine)

    static QAtomicInt m_sessionIdCounter;

public: // types
    class ParserData
    {
    public:
        QScopedPointer<QScxmlDataModel> m_ownedDataModel;
        QVector<QScxmlError> m_errors;
    };

public:
    QScxmlStateMachinePrivate();
    ~QScxmlStateMachinePrivate();

    static QScxmlStateMachinePrivate *get(QScxmlStateMachine *t)
    { return t->d_func(); }

    void setQStateMachine(QScxmlInternal::WrappedQStateMachine *stateMachine);

    QAbstractState *stateByScxmlName(const QString &scxmlName);

    ParserData *parserData();

    void setIsInvoked(bool invoked)
    { m_isInvoked = invoked; }

    const QVector<QScxmlInvokableService *> &invokedServices() const
    { return m_invokedServices; }

    void addService(QScxmlInvokableService *service);

    bool removeService(QScxmlInvokableService *service);

    bool executeInitialSetup();

public: // types & data fields:
    QString m_sessionId;
    bool m_isInvoked;
    QScxmlDataModel *m_dataModel;
    QScxmlStateMachine::BindingMethod m_dataBinding;
    QScxmlExecutableContent::QScxmlExecutionEngine *m_executionEngine;
    QScxmlTableData *m_tableData;
    QScxmlEvent m_event;
    QScxmlInternal::WrappedQStateMachine *m_qStateMachine;
    QScxmlEventFilter *m_eventFilter;
    QVector<QScxmlState*> m_statesToInvoke;
    QScxmlStateMachine *m_parentStateMachine;

private:
    QVector<QScxmlInvokableService *> m_invokedServices;
    QScopedPointer<ParserData> m_parserData; // used when created by StateMachine::fromFile.
};

QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_P_H

