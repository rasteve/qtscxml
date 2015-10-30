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

/*!
 * \class QScxmlDataModel
 * \brief Data-model base-class for a QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * One data-model can only belong to one state-machine.
 *
 * \sa QScxmlStateMachine QScxmlCppDataModel QScxmlEcmaScriptDataModel QScxmlNullDataModel
 */

QScxmlDataModel::QScxmlDataModel()
    : d(new QScxmlDataModelPrivate)
{}

QScxmlDataModel::~QScxmlDataModel()
{
    delete d;
}

/*!
 * \return the state-machine to which this data-model is associated.
 */
QScxmlStateMachine *QScxmlDataModel::stateMachine() const
{
    return d->m_stateMachine;
}

QScxmlDataModel *QScxmlDataModelPrivate::instantiateDataModel(
        DocumentModel::Scxml::DataModelType type, QScxmlStateMachine *stateMachine)
{
    Q_ASSERT(stateMachine);

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

void QScxmlDataModelPrivate::setStateMachine(QScxmlStateMachine *stateMachine)
{
    m_stateMachine = stateMachine;
}
