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

#ifndef SCXMLEVENT_P_H
#define SCXMLEVENT_P_H

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

#include "scxmlevent.h"
#include "scxmlstatemachine.h"
#include "executablecontent_p.h"

#include <QAtomicInt>

QT_BEGIN_NAMESPACE

namespace Scxml {

class EventBuilder
{
    StateMachine* table;
    ExecutableContent::StringId instructionLocation;
    QByteArray event;
    EvaluatorId eventexpr;
    QString contents;
    EvaluatorId contentExpr;
    const ExecutableContent::Array<ExecutableContent::Param> *params;
    QScxmlEvent::EventType eventType;
    QByteArray id;
    QString idLocation;
    QString target;
    EvaluatorId targetexpr;
    QString type;
    EvaluatorId typeexpr;
    const ExecutableContent::Array<ExecutableContent::StringId> *namelist;

    static QAtomicInt idCounter;
    QByteArray generateId() const
    {
        QByteArray id = QByteArray::number(++idCounter);
        id.prepend("id-");
        return id;
    }

    EventBuilder()
    { init(); }

    void init() // Because stupid VS2012 can't cope with non-static field initializers.
    {
        table = Q_NULLPTR;
        eventexpr = NoEvaluator;
        contentExpr = NoEvaluator;
        params = Q_NULLPTR;
        eventType = QScxmlEvent::ExternalEvent;
        targetexpr = NoEvaluator;
        typeexpr = NoEvaluator;
        namelist = Q_NULLPTR;
    }

public:
    EventBuilder(StateMachine *table, const QString &eventName, const ExecutableContent::DoneData *doneData)
    {
        init();
        this->table = table;
        Q_ASSERT(doneData);
        instructionLocation = doneData->location;
        event = eventName.toUtf8();
        contents = table->tableData()->string(doneData->contents);
        contentExpr = doneData->expr;
        params = &doneData->params;
    }

    EventBuilder(StateMachine *table, ExecutableContent::Send &send)
    {
        init();
        this->table = table;
        instructionLocation = send.instructionLocation;
        event = table->tableData()->byteArray(send.event);
        eventexpr = send.eventexpr;
        contents = table->tableData()->string(send.content);
        params = send.params();
        id = table->tableData()->byteArray(send.id);
        idLocation = table->tableData()->string(send.idLocation);
        target = table->tableData()->string(send.target);
        targetexpr = send.targetexpr;
        type = table->tableData()->string(send.type);
        typeexpr = send.typeexpr;
        namelist = &send.namelist;
    }

    QScxmlEvent *operator()() { return buildEvent(); }

    QScxmlEvent *buildEvent();

    static QScxmlEvent *errorEvent(const QByteArray &name, const QByteArray &sendid)
    {
        EventBuilder event;
        event.event = name;
        event.eventType = QScxmlEvent::PlatformEvent; // Errors are platform events. See e.g. test331.
        // _event.data == null, see test528
        event.id = sendid;
        return event();
    }
};

} // Scxml namespace

class QScxmlEventPrivate
{
public:
    QByteArray name;
    QScxmlEvent::EventType eventType;
    QVariantList dataValues; // extra data
    QStringList dataNames; // extra data
    QByteArray sendid; // if set, or id of <send> if failure
    QString origin; // uri to answer by setting the target of send, empty for internal and platform events
    QString originType; // type to answer by setting the type of send, empty for internal and platform events
    QByteArray invokeId; // id of the invocation that triggered the child process if this was invoked
};

QT_END_NAMESPACE

#endif // SCXMLEVENT_P_H

