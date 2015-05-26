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

#include "scxmlevent.h"
#include "scxmlstatetable.h"
#include "executablecontent_p.h"

#include <QAtomicInt>

namespace Scxml {

class EventBuilder
{
    StateTable* table = nullptr;
    ExecutableContent::StringId instructionLocation;
    QByteArray event;
    DataModel::EvaluatorId eventexpr = DataModel::NoEvaluator;
    QString contents;
    DataModel::EvaluatorId contentExpr = DataModel::NoEvaluator;
    const ExecutableContent::Array<ExecutableContent::Param> *params = nullptr;
    ScxmlEvent::EventType eventType = ScxmlEvent::External;
    QByteArray id;
    QString idLocation;
    QString target;
    DataModel::EvaluatorId targetexpr = DataModel::NoEvaluator;
    QString type;
    DataModel::EvaluatorId typeexpr = DataModel::NoEvaluator;
    const ExecutableContent::Array<ExecutableContent::StringId> *namelist = nullptr;

    static QAtomicInt idCounter;
    QByteArray generateId() const
    {
        QByteArray id = QByteArray::number(++idCounter);
        id.prepend("id-");
        return id;
    }

    EventBuilder()
    {}

public:
    EventBuilder(StateTable *table, const QString &eventName, const ExecutableContent::DoneData *doneData)
        : table(table)
    {
        Q_ASSERT(doneData);
        instructionLocation = doneData->location;
        event = eventName.toUtf8();
        contents = table->executionEngine()->string(doneData->contents);
        contentExpr = doneData->expr;
        params = &doneData->params;
    }

    EventBuilder(StateTable *table, ExecutableContent::Send &send)
        : table(table)
        , instructionLocation(send.instructionLocation)
        , event(table->executionEngine()->byteArray(send.event))
        , eventexpr(send.eventexpr)
        , contents(table->executionEngine()->string(send.content))
        , params(send.params())
        , id(table->executionEngine()->byteArray(send.id))
        , idLocation(table->executionEngine()->string(send.idLocation))
        , target(table->executionEngine()->string(send.target))
        , targetexpr(send.targetexpr)
        , type(table->executionEngine()->string(send.type))
        , typeexpr(send.typeexpr)
        , namelist(&send.namelist)
    {}

    ScxmlEvent *operator()() { return buildEvent(); }

    ScxmlEvent *buildEvent();

    static ScxmlEvent *errorEvent(const QByteArray &name, const QByteArray &sendid)
    {
        EventBuilder event;
        event.event = name;
        event.eventType = ScxmlEvent::Platform; // Errors are platform events. See e.g. test331.
        // _event.data == null, see test528
        event.id = sendid;
        return event();
    }
};

} // Scxml namespace

#endif // SCXMLEVENT_P_H

