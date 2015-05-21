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

#include "executablecontent_p.h"
#include "scxmlevent_p.h"

using namespace Scxml;

static bool evaluate(const ExecutableContent::Param &param, StateTable *table, QVariantList &dataValues, QStringList &dataNames)
{
    auto dataModel = table->dataModel();
    auto ee = table->executionEngine();
    if (param.expr != DataModel::NoEvaluator) {
        bool success = false;
        auto v = dataModel->evaluateToVariant(param.expr, &success);
        dataValues.append(v);
        dataNames.append(ee->string(param.name));
        return success;
    }

    QString loc;
    if (param.location != ExecutableContent::NoString) {
        loc = ee->string(param.location);
    }

    if (loc.isEmpty()) {
        return false;
    }

    if (dataModel->hasProperty(loc)) {
        dataValues.append(dataModel->property(loc));
        dataNames.append(ee->string(param.name));
        return true;
    } else {
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in <param>: %1 is not a valid location")
                           .arg(loc),
                           /*sendid =*/ QByteArray());
        return false;
    }
}

static bool evaluate(const ExecutableContent::Array<ExecutableContent::Param> *params, StateTable *table, QVariantList &dataValues, QStringList &dataNames)
{
    if (!params)
        return true;

    auto paramPtr = params->const_data();
    for (qint32 i = 0; i != params->count; ++i, ++paramPtr) {
        if (!evaluate(*paramPtr, table, dataValues, dataNames))
            return false;
    }

    return true;
}

QAtomicInt EventBuilder::idCounter = QAtomicInt(0);

ScxmlEvent *EventBuilder::buildEvent()
{
    auto dataModel = table ? table->dataModel() : nullptr;
    auto engine = table ? table->executionEngine() : nullptr;

    QByteArray eventName = event;
    bool ok = true;
    if (eventexpr != DataModel::NoEvaluator) {
        eventName = dataModel->evaluateToString(eventexpr, &ok).toUtf8();
        ok = true; // ignore failure.
    }

    QVariantList dataValues;
    QStringList dataNames;
    if ((!params || params->count == 0) && (!namelist || namelist->count == 0)) {
        QVariant data;
        if (contentExpr != DataModel::NoEvaluator) {
            data = dataModel->evaluateToString(contentExpr, &ok);
        } else {
            data = contents;
        }
        if (ok) {
            dataValues.append(data);
        } else {
            // expr evaluation failure results in the data property of the event being set to null. See e.g. test528.
            dataValues = { QVariant(QMetaType::VoidStar, 0) };
        }
    } else {
        if (evaluate(params, table, dataValues, dataNames)) {
            if (namelist) {
                for (qint32 i = 0; i < namelist->count; ++i) {
                    QString name = engine->string(namelist->const_data()[i]);
                    dataNames << name;
                    dataValues << dataModel->property(name);
                }
            }
        } else {
            // If the evaluation of the <param> tags fails, set _event.data to an empty string.
            // See test343.
            dataValues = { QVariant(QMetaType::VoidStar, 0) };
            dataNames.clear();
        }
    }

    QByteArray sendid = id;
    if (!idLocation.isEmpty()) {
        sendid = generateId();
        table->dataModel()->setStringProperty(idLocation, QString::fromUtf8(sendid), engine->string(instructionLocation), &ok);
        if (!ok)
            return nullptr;
    }

    QString origin = target;
    if (targetexpr != DataModel::NoEvaluator) {
        origin = dataModel->evaluateToString(targetexpr, &ok);
        if (!ok)
            return nullptr;
    }
    if (origin.isEmpty()) {
        if (eventType == ScxmlEvent::External) {
            origin = QStringLiteral("#_internal");
        }
    } else if (!table->isLegalTarget(origin)) {
        // [6.2.4] and test194.
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in %1: %2 is not a legal target")
                           .arg(engine->string(instructionLocation), origin),
                           sendid);
        return nullptr;
    } else if (!table->isDispatchableTarget(origin)) {
        // [6.2.4] and test521.
        table->submitError(QByteArray("error.communication"),
                           QStringLiteral("Error in %1: cannot dispatch to target '%2'")
                           .arg(engine->string(instructionLocation), origin),
                           sendid);
        return nullptr;
    }

    QString origintype = type;
    if (origintype.isEmpty()) {
        // [6.2.5] and test198
        origintype = QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor");
    }
    if (typeexpr != DataModel::NoEvaluator) {
        origintype = dataModel->evaluateToString(typeexpr, &ok);
        if (!ok)
            return nullptr;
    }
    if (!origintype.isEmpty() && origintype != QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor")) {
        // [6.2.5] and test199
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in %1: %2 is not a valid type")
                           .arg(engine->string(instructionLocation), origintype),
                           sendid);
        return nullptr;
    }

    return new ScxmlEvent(eventName, eventType, dataValues, dataNames, sendid, origin, origintype);
}
