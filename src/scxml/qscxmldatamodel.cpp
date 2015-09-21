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

#include "qscxmldatamodel_p.h"
#include "qscxmlnulldatamodel.h"
#include "qscxmlecmascriptdatamodel.h"
#include "qscxmlstatemachine_p.h"

QT_USE_NAMESPACE

QScxmlDataModel::ForeachLoopBody::~ForeachLoopBody()
{}

QScxmlDataModel::QScxmlDataModel()
    : d(new QScxmlDataModelPrivate)
{}

QScxmlDataModel::~QScxmlDataModel()
{
    delete d;
}

QScxmlStateMachine *QScxmlDataModel::stateMachine() const
{
    return d->m_stateMachine;
}

void QScxmlDataModel::setTable(QScxmlStateMachine *table)
{
    d->m_stateMachine = table;
}

QScxmlDataModel *QScxmlDataModelPrivate::instantiateDataModel(
        DocumentModel::Scxml::DataModelType type, QScxmlStateMachine *stateMachine)
{
    QScxmlDataModel *dataModel = Q_NULLPTR;
    switch (type) {
    case DocumentModel::Scxml::NullDataModel:
        dataModel = new QScxmlNullDataModel;
        break;
    case DocumentModel::Scxml::JSDataModel:
        dataModel = new QScxmlEcmaScriptDataModel;
        break;
    case DocumentModel::Scxml::CppDataModel:
        break;
    default:
        Q_UNREACHABLE();
    }
    stateMachine->setDataModel(dataModel);
    QScxmlStateMachinePrivate::get(stateMachine)->parserData()->m_ownedDataModel.reset(dataModel);

    return dataModel;
}
