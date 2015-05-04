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

#ifndef ECMASCRIPTPLATFORMPROPERTIES_H
#define ECMASCRIPTPLATFORMPROPERTIES_H

#include "scxmlglobals.h"

#include <QJSValue>
#include <QObject>

namespace Scxml {

class StateTable;
class SCXML_EXPORT PlatformProperties: public QObject
{
    Q_OBJECT

    PlatformProperties &operator=(const PlatformProperties &) = delete;

    PlatformProperties(QObject *parent);

    Q_PROPERTY(QString marks READ marks CONSTANT)

public:
    static PlatformProperties *create(QJSEngine *engine, StateTable *table);

    QJSEngine *engine() const;
    StateTable *table() const;
    QJSValue jsValue() const;

    QString marks() const;

    Q_INVOKABLE bool In(const QString &stateName);

private:
    class Data;
    Data *data;
};

} // Scxml namespace

#endif // ECMASCRIPTPLATFORMPROPERTIES_H
