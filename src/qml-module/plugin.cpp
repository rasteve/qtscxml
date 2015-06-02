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

#include "statemachine.h"
#include "state.h"

#include <QQmlExtensionPlugin>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class ScxmlStateMachinePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.Scxml/1.0")

public:
    void registerTypes(const char *uri)
    {
        qmlRegisterType<StateMachine>(uri, 1, 0, "StateMachine");
        qmlRegisterType<State>(uri, 1, 0, "State");
//        qmlRegisterType<QHistoryState>(uri, 1, 0, "HistoryState");
//        qmlRegisterType<FinalState>(uri, 1, 0, "FinalState");
//        qmlRegisterUncreatableType<QState>(uri, 1, 0, "QState", "Don't use this, use State instead");
//        qmlRegisterUncreatableType<QAbstractState>(uri, 1, 0, "QAbstractState", "Don't use this, use State instead");
//        qmlRegisterUncreatableType<QSignalTransition>(uri, 1, 0, "QSignalTransition", "Don't use this, use SignalTransition instead");
//        qmlRegisterType<SignalTransition>(uri, 1, 0, "SignalTransition");
//        qmlRegisterType<TimeoutTransition>(uri, 1, 0, "TimeoutTransition");
        qmlProtectModule(uri, 1);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
