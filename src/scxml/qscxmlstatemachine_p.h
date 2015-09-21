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
    int eventIdForDelayedEvent(const QByteArray &scxmlEventId);

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

public: // types & data fields:
    QString m_sessionId;
    bool m_isInvoked;
    QScxmlDataModel *m_dataModel;
    QScxmlStateMachine::BindingMethod m_dataBinding;
    QScxmlExecutableContent::ExecutionEngine *m_executionEngine;
    QScxmlTableData *m_tableData;
    QScxmlEvent m_event;
    QScxmlInternal::WrappedQStateMachine *m_qStateMachine;
    QScxmlEventFilter *m_eventFilter;
    QVector<QScxmlState*> m_statesToInvoke;
    QVector<QScxmlInvokableService *> m_invokedServices;
    QScxmlStateMachine *m_parentStateMachine;

private:
    QScopedPointer<ParserData> m_parserData; // used when created by StateMachine::fromFile.
};

QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_P_H

