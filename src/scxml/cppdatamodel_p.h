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

#ifndef CPPDATAMODEL_P_H
#define CPPDATAMODEL_P_H

#include "cppdatamodel.h"
#include "QScxmlEvent"

QT_BEGIN_NAMESPACE

namespace Scxml {

class Q_SCXML_EXPORT QScxmlCppDataModelPrivate
{
public:
    QScxmlEvent event;
};

} // Scxml namespace

QT_END_NAMESPACE

#endif // CPPDATAMODEL_P_H
