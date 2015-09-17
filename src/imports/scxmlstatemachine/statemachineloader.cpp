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

#include "statemachineloader.h"

#include <QtScxml/scxmlstatemachine.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlFile>
#include <QBuffer>

StateMachineLoader::StateMachineLoader(QObject *parent)
    : QObject(parent)
    , m_stateMachine(Q_NULLPTR)
{
}

QT_PREPEND_NAMESPACE(Scxml::QScxmlStateMachine) *StateMachineLoader::stateMachine() const
{
    return m_stateMachine;
}

QUrl StateMachineLoader::filename()
{
    return m_filename;
}

void StateMachineLoader::setFilename(const QUrl &filename)
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

bool StateMachineLoader::parse(const QUrl &filename)
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
    m_stateMachine = Scxml::QScxmlStateMachine::fromData(&buf);
    m_stateMachine->setParent(this);
    m_stateMachine->init();

    if (m_stateMachine->errors().isEmpty()) {
        emit stateMachineChanged();
        QMetaObject::invokeMethod(m_stateMachine, "start", Qt::QueuedConnection);
        return true;
    } else {
        qmlInfo(this) << QStringLiteral("Something went wrong while parsing '%1':").arg(filename.fileName()) << endl;
        foreach (const Scxml::QScxmlError &msg, m_stateMachine->errors()) {
            qmlInfo(this) << msg.toString();
        }

        emit stateMachineChanged();
        return false;
    }
}
