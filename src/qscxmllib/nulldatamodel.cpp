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

#include "nulldatamodel.h"

using namespace Scxml;

NullDataModel::NullDataModel(StateTable *table)
    : DataModel(table)
{}

void NullDataModel::setEvaluators(const EvaluatorInfos &evals, const AssignmentInfos &assignments, const ForeachInfos &foreaches)
{
    // FIXME: bool evaluators need to be stored.
}

void NullDataModel::setup(const ExecutableContent::StringIds &dataItemNames)
{
    Q_UNUSED(dataItemNames);
}

QString NullDataModel::evaluateToString(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
    return QString();
}

bool NullDataModel::evaluateToBool(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
    return false;
}

QVariant NullDataModel::evaluateToVariant(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
    return QVariant();
}

void NullDataModel::evaluateToVoid(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void NullDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool NullDataModel::evaluateForeach(EvaluatorId id, bool *ok, std::function<bool ()> body)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNUSED(body);
    Q_UNREACHABLE();
    return false;
}

void NullDataModel::setEvent(const ScxmlEvent &event)
{
    Q_UNUSED(event);
}

QVariant NullDataModel::property(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

bool NullDataModel::hasProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

void NullDataModel::setStringProperty(const QString &name, const QString &value, const QString &context, bool *ok)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}
