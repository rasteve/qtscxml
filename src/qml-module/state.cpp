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

#include <QScxmlLib/scxmlstatetable.h>
#include <QQmlInfo>

State::State(StateMachine *parent)
    : QObject(parent)
{}

void State::componentComplete()
{
    if (Scxml::StateTable *table = qobject_cast<StateMachine *>(parent())->stateMachine()) {
        if (Scxml::ScxmlState *state = table->findState(m_scxmlName)) {
            if (state != m_state) {
                m_state = state;
                connect(m_state, SIGNAL(activeChanged(bool)), this, SIGNAL(activeChanged(bool)));
                connect(m_state, SIGNAL(didEnter()), this, SIGNAL(didEnter()));
                connect(m_state, SIGNAL(willExit()), this, SIGNAL(willExit()));
            }
        }
    }

    if (m_state == nullptr)
        qmlInfo(this) << QStringLiteral("No state '%1' found.").arg(m_scxmlName);
}

bool State::isActive() const
{
    return m_state && m_state->active();
}

QString State::scxmlName() const
{
    return m_scxmlName;
}

void State::setScxmlName(const QString &scxmlName)
{
    if (m_scxmlName != scxmlName) {
        m_scxmlName = scxmlName;
        emit scxmlNameChanged();
    }
}
