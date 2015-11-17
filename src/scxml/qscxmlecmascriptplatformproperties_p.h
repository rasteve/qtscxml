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

#include "qscxmlglobals.h"

#include <QJSValue>
#include <QObject>

QT_BEGIN_NAMESPACE

class QScxmlStateMachine;
class QScxmlPlatformProperties: public QObject
{
    Q_OBJECT

    QScxmlPlatformProperties(QObject *parent);

    Q_PROPERTY(QString marks READ marks CONSTANT)

public:
    static QScxmlPlatformProperties *create(QJSEngine *engine, QScxmlStateMachine *stateMachine);
    ~QScxmlPlatformProperties();

    QJSEngine *engine() const;
    QScxmlStateMachine *stateMachine() const;
    QJSValue jsValue() const;

    QString marks() const;

    Q_INVOKABLE bool In(const QString &stateName);

private:
    class Data;
    Data *data;
};

QT_END_NAMESPACE

#endif // ECMASCRIPTPLATFORMPROPERTIES_P_H
