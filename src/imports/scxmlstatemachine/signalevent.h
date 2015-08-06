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

#ifndef SIGNALEVENT_H
#define SIGNALEVENT_H

#include <QJSValue>
#include <QObject>

QT_BEGIN_NAMESPACE

namespace Scxml {
class ScxmlState;
}

class StateMachine;
class SignalEvent: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue signal READ signal WRITE setSignal NOTIFY signalChanged)
    Q_PROPERTY(QString eventName READ eventName WRITE setEventName NOTIFY eventNameChanged)

public:
    explicit SignalEvent(StateMachine *parent = 0);

    QString eventName() const;
    void setEventName(const QString &eventName);

    const QJSValue &signal();
    void setSignal(const QJSValue &signal);

Q_SIGNALS:
    void signalChanged();
    void eventNameChanged();

public Q_SLOTS:
    void invoke();

private:
    QJSValue m_signal;
    QByteArray m_eventName;
    QMetaObject::Connection m_signalConnection;
};

QT_END_NAMESPACE

#endif // SIGNALEVENT_H
