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

#include "nodatamodel.h"

using namespace Scxml;

NoDataModel::NoDataModel(StateTable *table)
    : DataModel(table)
{}

DataModel::EvaluatorString NoDataModel::createEvaluator(const QString &expr, const QString &context)
{
    Q_UNUSED(expr);
    Q_UNUSED(context);
    Q_UNREACHABLE(); // FIXME: give an error.
}
