/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "substatemachines.h"

QT_BEGIN_NAMESPACE

QScxmlSubStateMachines::QScxmlSubStateMachines(QObject *parent) : QObject(parent)
{
}

QVariantMap QScxmlSubStateMachines::children()
{
    QVariantMap ret;
    if (m_stateMachine) {
        const QVector<QScxmlStateMachine *> children = m_stateMachine->runningSubStateMachines();
        for (QScxmlStateMachine *stateMachine : children)
            ret.insertMulti(stateMachine->name(), QVariant::fromValue(stateMachine));
    }
    return ret;
}

void QScxmlSubStateMachines::classBegin()
{
}

QScxmlStateMachine *QScxmlSubStateMachines::stateMachine() const
{
    return m_stateMachine;
}

void QScxmlSubStateMachines::setStateMachine(QScxmlStateMachine *stateMachine)
{
    if (stateMachine != m_stateMachine) {
        if (m_stateMachine) {
            disconnect(m_stateMachine, &QScxmlStateMachine::runningSubStateMachinesChanged,
                       this, &QScxmlSubStateMachines::childrenChanged);
        }
        m_stateMachine = stateMachine;
        connect(m_stateMachine, &QScxmlStateMachine::runningSubStateMachinesChanged,
                this, &QScxmlSubStateMachines::childrenChanged);
        emit stateMachineChanged();
        emit childrenChanged();
    }
}

QQmlListProperty<QObject> QScxmlSubStateMachines::qmlChildren()
{
    return QQmlListProperty<QObject>(this, m_qmlChildren);
}


void QScxmlSubStateMachines::componentComplete()
{
    if (!m_stateMachine) {
        if ((m_stateMachine = qobject_cast<QScxmlStateMachine *>(parent()))) {
            connect(m_stateMachine, &QScxmlStateMachine::runningSubStateMachinesChanged,
                    this, &QScxmlSubStateMachines::childrenChanged);
        }
    }
}

QT_END_NAMESPACE
