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

#include "statemachineloader.h"

#include <QQmlExtensionPlugin>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QScxmlStateMachinePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.Scxml/1.0")

public:
    void registerTypes(const char *uri)
    {
        qmlRegisterType<QScxmlStateMachineLoader>(uri, 1, 0, "StateMachineLoader");
        qmlProtectModule(uri, 1);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
