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

void NullDataModel::setup()
{}

void NullDataModel::initializeDataFor(QState *state)
{
    Q_UNUSED(state);
}

DataModel::ToStringEvaluator NullDataModel::createToStringEvaluator(const QString &expr, const QString &context)
{
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
}

DataModel::ToBoolEvaluator NullDataModel::createToBoolEvaluator(const QString &expr, const QString &context)
{
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
}

DataModel::ToVariantEvaluator NullDataModel::createToVariantEvaluator(const QString &expr, const QString &context)
{
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
}

DataModel::ToVoidEvaluator NullDataModel::createScriptEvaluator(const QString &expr, const QString &context)
{
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
}

DataModel::ToVoidEvaluator NullDataModel::createAssignmentEvaluator(const QString &dest, const QString &expr, const QString &context)
{
    Q_UNUSED(dest);
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
}

DataModel::ForeachEvaluator NullDataModel::createForeachEvaluator(const QString &array, const QString &item, const QString &index, const QString &context)
{
    Q_UNUSED(array);
    Q_UNUSED(item);
    Q_UNUSED(index);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return nullptr;
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
