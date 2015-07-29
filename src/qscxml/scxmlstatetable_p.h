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

#include "executablecontent_p.h"
#include "scxmlstatetable.h"

#include <QtCore/private/qstatemachine_p.h>

namespace Scxml {

class StateTablePrivate: public QStateMachinePrivate
{
    Q_DECLARE_PUBLIC(StateTable)

    static QAtomicInt m_sessionIdCounter;

public: // types
    class ParserData
    {
    public:
        QScopedPointer<DataModel> m_ownedDataModel;
        QVector<ScxmlError> m_errors;
    };

public:
    StateTablePrivate();
    ~StateTablePrivate();

    static StateTablePrivate *get(StateTable *t)
    { return t->d_func(); }

    ParserData *parserData();

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
    const int m_sessionId;
    DataModel *m_dataModel;
    ExecutableContent::ContainerId m_initialSetup;
    StateTable::BindingMethod m_dataBinding;
    ExecutableContent::ExecutionEngine *m_executionEngine;
    TableData *m_tableData;
    ScxmlEvent m_event;
    QString m_name;

    struct QueuedEvent
    {
        QueuedEvent(QEvent *event = Q_NULLPTR, StateTable::EventPriority priority = StateTable::NormalPriority)
            : event(event)
            , priority(priority)
        {}

        QEvent *event;
        StateTable::EventPriority priority;
    };
    QVector<QueuedEvent> *m_queuedEvents;

private:
    QScopedPointer<ParserData> m_parserData; // used when created by StateTable::fromFile.
};

} // Scxml namespace

#endif // SCXMLSTATETABLE_P_H

