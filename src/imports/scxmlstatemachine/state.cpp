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

#include "state.h"
#include "statemachine.h"

#include <QScxml/scxmlstatetable.h>
#include <QQmlInfo>

State::State(StateMachine *parent)
    : QObject(parent)
    , completed(false)
    , active(false)
{}

void State::componentComplete()
{
    completed = true;
    establishConnections();

    StateMachine *stateMachine = qobject_cast<StateMachine *>(parent());
    if (Scxml::StateTable *table = stateMachine->stateMachine()) {
        active = table->isActive(m_scxmlName);
        connect(stateMachine, SIGNAL(filenameChanged()), this, SLOT(onFilenameChanged()));
    }
    if (active)
        emit activeChanged(active);
}

bool State::isActive() const
{
    return active;
}

QString State::scxmlName() const
{
    return m_scxmlName;
}

void State::setScxmlName(const QString &scxmlName)
{
    if (m_scxmlName != scxmlName) {
        m_scxmlName = scxmlName;
        breakConnections();
        establishConnections();
        emit scxmlNameChanged();
    }
}

void State::setActive(bool active)
{
    this->active = active;
    emit activeChanged(active);
}

void State::onFilenameChanged()
{
    breakConnections();
    establishConnections();
}

void State::breakConnections()
{
    disconnect(activeConnection);
    disconnect(didEnterConnection);
    disconnect(willExitConnection);
}

void State::establishConnections()
{
    if (!completed)
        return;

    Scxml::StateTable *table = qobject_cast<StateMachine *>(parent())->stateMachine();
    if (table == Q_NULLPTR) {
        return;
    }

    if (!table->hasState(m_scxmlName)) {
        qmlInfo(this) << QStringLiteral("No state '%1' found.").arg(m_scxmlName);
        return;
    }

    activeConnection = table->connect(m_scxmlName, SIGNAL(activeChanged(bool)), this, SLOT(setActive(bool)));
    didEnterConnection = table->connect(m_scxmlName, SIGNAL(didEnter()), this, SIGNAL(didEnter()));
    willExitConnection = table->connect(m_scxmlName, SIGNAL(willExit()), this, SIGNAL(willExit()));
}
