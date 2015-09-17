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

#include "qscxmlcppdatamodel_p.h"
#include "qscxmlstatemachine.h"

QT_USE_NAMESPACE

using namespace QScxmlExecutableContent;

QScxmlCppDataModel::QScxmlCppDataModel()
    : data(new QScxmlCppDataModelPrivate)
{}

QScxmlCppDataModel::~QScxmlCppDataModel()
{
    delete data;
}

void QScxmlCppDataModel::setup(const QVariantMap &initialDataValues)
{
    Q_UNUSED(initialDataValues);
}

void QScxmlCppDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void QScxmlCppDataModel::evaluateInitialization(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool QScxmlCppDataModel::evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNUSED(body);
    Q_UNREACHABLE();
    return false;
}

void QScxmlCppDataModel::setEvent(const QScxmlEvent &event)
{
    data->event = event;
}

const QScxmlEvent &QScxmlCppDataModel::event() const
{
    return data->event;
}

QVariant QScxmlCppDataModel::property(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

bool QScxmlCppDataModel::hasProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

void QScxmlCppDataModel::setProperty(const QString &name, const QVariant &value, const QString &context, bool *ok)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool QScxmlCppDataModel::In(const QString &stateName) const
{
    return stateMachine()->isActive(stateName);
}
