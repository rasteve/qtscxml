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

#include "signalevent.h"
#include "statemachine.h"

#include <QQmlContext>
#include <QQmlInfo>
#include <QQmlEngine>

#include <private/qjsvalue_p.h>
#include <private/qv8engine_p.h>

SignalEvent::SignalEvent(StateMachine *parent)
    : QObject(parent)
{}

QString SignalEvent::eventName() const
{
    return QString::fromUtf8(m_eventName);
}

void SignalEvent::setEventName(const QString &eventName)
{
    QByteArray name = eventName.toUtf8();
    if (name != m_eventName) {
        m_eventName = name;
        emit eventNameChanged();
    }
}

const QJSValue &SignalEvent::signal()
{
    return m_signal;
}

void SignalEvent::setSignal(const QJSValue &signal)
{
    if (m_signal.strictlyEquals(signal))
        return;

    disconnect(m_signalConnection);

    m_signal = signal;

    QV4::ExecutionEngine *jsEngine = QV8Engine::getV4(QQmlEngine::contextForObject(this)->engine());
    QV4::Scope scope(jsEngine);

    QV4::Scoped<QV4::QObjectMethod> qobjectSignal(scope, QJSValuePrivate::convertedToValue(jsEngine, m_signal));
    Q_ASSERT(qobjectSignal);

    QObject *sender = qobjectSignal->object();
    QByteArray signature = sender->metaObject()->method(qobjectSignal->methodIndex()).methodSignature();
    signature.prepend(SIGNAL());
    m_signalConnection = connect(sender, signature.constData(), this, SLOT(invoke()));

    emit signalChanged();
}

void SignalEvent::invoke()
{
    if (Scxml::StateTable *table = qobject_cast<StateMachine *>(parent())->stateMachine()) {
        table->submitEvent(m_eventName);
    } else {
        qmlInfo(this) << QStringLiteral("No state table found to submit event '%1'.").arg(QString::fromUtf8(m_eventName));
    }
}
