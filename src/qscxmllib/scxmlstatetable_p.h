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

#ifndef SCXMLSTATETABLE_P_H
#define SCXMLSTATETABLE_P_H

#include "executablecontent_p.h"
#include "scxmlstatetable.h"

#include <QtCore/private/qstatemachine_p.h>

namespace Scxml {

class StateTablePrivate: public QStateMachinePrivate
{
    Q_DECLARE_PUBLIC(StateTable)

    static QAtomicInt m_sessionIdCounter;

public:
    StateTablePrivate();
    ~StateTablePrivate();

protected: // overrides for QStateMachinePrivate:
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    void noMicrostep() Q_DECL_OVERRIDE;
    void processedPendingEvents(bool didChange) Q_DECL_OVERRIDE;
    void beginMacrostep() Q_DECL_OVERRIDE;
    void endMacrostep(bool didChange) Q_DECL_OVERRIDE;

    void emitStateFinished(QState *forState, QFinalState *guiltyState) Q_DECL_OVERRIDE;
    void startupHook() Q_DECL_OVERRIDE;
#endif

    int eventIdForDelayedEvent(const QByteArray &scxmlEventId);

public: // StateTable data fields:
    DataModel *m_dataModel = nullptr;
    const int m_sessionId;
    ExecutableContent::ContainerId m_initialSetup = ExecutableContent::NoInstruction;
    QJSEngine *m_engine = nullptr;
    StateTable::BindingMethod m_dataBinding = StateTable::EarlyBinding;
    ExecutableContent::ExecutionEngine *m_executionEngine = nullptr;
    TableData *tableData = nullptr;
    ScxmlEvent _event;
    QString _name;
    ExecutableContent::StringIds dataItemNames;

    struct QueuedEvent { QEvent *event; StateTable::EventPriority priority; };
    QVector<QueuedEvent> *m_queuedEvents = nullptr;
};

} // Scxml namespace

#endif // SCXMLSTATETABLE_P_H

