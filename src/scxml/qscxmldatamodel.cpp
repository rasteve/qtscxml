/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScxml module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

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
 * SCXML data models are described in the SCXML specification.
 *
 * One data-model can only belong to one state machine.
 *
 * \sa QScxmlStateMachine QScxmlCppDataModel QScxmlEcmaScriptDataModel QScxmlNullDataModel
 */

/*!
 * \brief Creates a new data model.
 */
QScxmlDataModel::QScxmlDataModel(QScxmlStateMachine *stateMachine)
    : d(new QScxmlDataModelPrivate(stateMachine))
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_dataModel = this;
}

/*!
 * \brief Destroys the data model.
 */
QScxmlDataModel::~QScxmlDataModel()
{
    delete d;
}

/*!
 * \return the state machine to which this data-model is associated.
 */
QScxmlStateMachine *QScxmlDataModel::stateMachine() const
{
    return d->m_stateMachine;
}

QScxmlTableData *QScxmlDataModel::tableData() const
{
    return stateMachine()->tableData();
}

QScxmlDataModel *QScxmlDataModelPrivate::instantiateDataModel(
        DocumentModel::Scxml::DataModelType type, QScxmlStateMachine *stateMachine)
{
    Q_ASSERT(stateMachine);

    QScxmlDataModel *dataModel = Q_NULLPTR;
    switch (type) {
    case DocumentModel::Scxml::NullDataModel:
        dataModel = new QScxmlNullDataModel(stateMachine);
        break;
    case DocumentModel::Scxml::JSDataModel:
        dataModel = new QScxmlEcmaScriptDataModel(stateMachine);
        break;
    case DocumentModel::Scxml::CppDataModel:
        break;
    default:
        Q_UNREACHABLE();
    }
    QScxmlStateMachinePrivate::get(stateMachine)->parserData()->m_ownedDataModel.reset(dataModel);

    return dataModel;
}

void QScxmlDataModelPrivate::setStateMachine(QScxmlStateMachine *stateMachine)
{
    m_stateMachine = stateMachine;
}

/*!
 * \fn QScxmlDataModel::setup(const QVariantMap &initialDataValues)
 *
 * Initializes the data model.
 */

/*!
 * \fn QScxmlDataModel::setEvent(const QScxmlEvent &event)
 *
 * Sets the event to use in subsequent executable content execution.
 */

/*!
 * \fn QScxmlDataModel::property(const QString &name) const
 *
 * Returns the value of the property \a name.
 */

/*!
 * \fn QScxmlDataModel::hasProperty(const QString &name) const
 *
 * \return Returns true if a property with the given \a name exists, false otherwise.
 */

/*!
 * \fn QScxmlDataModel::setProperty(const QString &name, const QVariant &value, const QString &context)
 *
 * Sets a property to the given value.
 *
 * \param name The name of the property to set.
 * \param value The new value for the property.
 * \param context A string that is used in error messages to indicate a location in the SCXML file
 *        where the error occurred.
 * \return Returns true if successful, otherwise false if any error occurred.
 */
