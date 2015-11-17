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

#include <QtScxml/qscxmlstatemachine.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlFile>
#include <QBuffer>

/*!
    \qmltype StateMachineLoader
    \instantiates QScxmlStateMachineLoader
    \inqmlmodule Scxml

    \brief Dynamically loads an SCXML file and instantiates the state machine.

    \since Scxml 1.0
 */

/*!
    \qmlsignal StateMachineLoader::filenameChanged()
    This signal is emitted when the user changes the filename.
*/

/*!
    \qmlsignal StateMachineLoader::stateMachineChanged()
*/

QScxmlStateMachineLoader::QScxmlStateMachineLoader(QObject *parent)
    : QObject(parent)
    , m_stateMachine(Q_NULLPTR)
{
}

/*!
    \qmlproperty QObject StateMachineLoader::stateMachine

    The state machine instance.
 */
QT_PREPEND_NAMESPACE(QScxmlStateMachine) *QScxmlStateMachineLoader::stateMachine() const
{
    return m_stateMachine;
}

/*!
    \qmlproperty string StateMachineLoader::filename

    The name of the SCXML file to load.
 */
QUrl QScxmlStateMachineLoader::filename()
{
    return m_filename;
}

void QScxmlStateMachineLoader::setFilename(const QUrl &filename)
{
    if (!filename.isValid())
        return;

    QUrl oldFilename = m_filename;
    if (m_stateMachine) {
        delete m_stateMachine;
        m_stateMachine = Q_NULLPTR;
    }

    if (parse(filename)) {
        m_filename = filename;
        emit filenameChanged();
    } else {
        m_filename.clear();
        if (!oldFilename.isEmpty()) {
            emit filenameChanged();
        }
    }
}

bool QScxmlStateMachineLoader::parse(const QUrl &filename)
{
    if (!QQmlFile::isSynchronous(filename)) {
        qmlInfo(this) << QStringLiteral("ERROR: cannot open '%1' for reading: only synchronous file access is supported.").arg(filename.fileName());
        return false;
    }
    QQmlFile scxmlFile(QQmlEngine::contextForObject(this)->engine(), filename);
    if (scxmlFile.isError()) {
        // the synchronous case can only fail when the file is not found (or not readable).
        qmlInfo(this) << QStringLiteral("ERROR: cannot open '%1' for reading.").arg(filename.fileName());
        return false;
    }

    QByteArray data(scxmlFile.dataByteArray());
    QBuffer buf(&data);
    Q_ASSERT(buf.open(QIODevice::ReadOnly));
    m_stateMachine = QScxmlStateMachine::fromData(&buf);
    m_stateMachine->setParent(this);
    m_stateMachine->init();

    if (m_stateMachine->errors().isEmpty()) {
        emit stateMachineChanged();
        QMetaObject::invokeMethod(m_stateMachine, "start", Qt::QueuedConnection);
        return true;
    } else {
        qmlInfo(this) << QStringLiteral("Something went wrong while parsing '%1':").arg(filename.fileName()) << endl;
        foreach (const QScxmlError &msg, m_stateMachine->errors()) {
            qmlInfo(this) << msg.toString();
        }

        emit stateMachineChanged();
        return false;
    }
}
