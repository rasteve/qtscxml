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

#ifndef ECMASCRIPTPLATFORMPROPERTIES_P_H
#define ECMASCRIPTPLATFORMPROPERTIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "scxmlglobals.h"

#include <QJSValue>
#include <QObject>

QT_BEGIN_NAMESPACE

namespace Scxml {

class QScxmlStateMachine;
class PlatformProperties: public QObject
{
    Q_OBJECT

    PlatformProperties(QObject *parent);

    Q_PROPERTY(QString marks READ marks CONSTANT)

public:
    static PlatformProperties *create(QJSEngine *engine, QScxmlStateMachine *stateMachine);
    ~PlatformProperties();

    QJSEngine *engine() const;
    QtNS::Scxml::QScxmlStateMachine *stateMachine() const;
    QJSValue jsValue() const;

    QString marks() const;

    Q_INVOKABLE bool In(const QString &stateName);

private:
    class Data;
    Data *data;
};

} // Scxml namespace

QT_END_NAMESPACE

#endif // ECMASCRIPTPLATFORMPROPERTIES_P_H
