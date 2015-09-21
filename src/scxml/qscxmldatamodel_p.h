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

#ifndef QSCXMLDATAMODEL_P_H
#define QSCXMLDATAMODEL_P_H

#include "qscxmldatamodel.h"
#include "qscxmlparser_p.h"

QT_BEGIN_NAMESPACE

class QScxmlDataModelPrivate
{
public:
    QScxmlDataModelPrivate()
        : m_stateMachine(Q_NULLPTR)
    {}

    static QScxmlDataModelPrivate *get(QScxmlDataModel *dataModel)
    { return dataModel->d; }

    static QScxmlDataModel *instantiateDataModel(DocumentModel::Scxml::DataModelType type,
                                                 QScxmlStateMachine *stateMachine);

    void setStateMachine(QScxmlStateMachine *stateMachine);

public:
    QScxmlStateMachine *m_stateMachine;
};

QT_END_NAMESPACE

#endif // QSCXMLDATAMODEL_P_H
