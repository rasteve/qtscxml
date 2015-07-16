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
#include "state.h"
#include "statemachine.h"

#include <QScxml/ecmascriptdatamodel.h>
#include <QScxml/scxmlstatetable.h>
#include <QScxml/scxmlparser.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlFile>
#include <QBuffer>

static void append(QQmlListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    if (State *state = qobject_cast<State *>(o)) {
        state->setParent(prop->object);
        static_cast<StateMachine::Kids *>(prop->data)->append(state);
        emit static_cast<StateMachine *>(prop->object)->statesChanged();
    } else if (SignalEvent *event = qobject_cast<SignalEvent *>(o)) {
        event->setParent(prop->object);
        static_cast<StateMachine::Kids *>(prop->data)->append(event);
        emit static_cast<StateMachine *>(prop->object)->statesChanged();
    }
}

static int count(QQmlListProperty<QObject> *prop)
{
    return static_cast<StateMachine::Kids *>(prop->data)->count();
}

static QObject *at(QQmlListProperty<QObject> *prop, int index)
{
    return static_cast<StateMachine::Kids *>(prop->data)->at(index);
}

static void clear(QQmlListProperty<QObject> *prop)
{
    static_cast<StateMachine::Kids *>(prop->data)->clear();
    emit static_cast<StateMachine *>(prop->object)->statesChanged();
}

StateMachine::StateMachine(QObject *parent)
    : QObject(parent)
    , m_table(Q_NULLPTR)
{
}

void StateMachine::componentComplete()
{
    if (m_table == Q_NULLPTR) {
         qmlInfo(this) << "No state machine loaded.";
         return;
    }

    bool ok = false;
    bool moreOk = QMetaObject::invokeMethod(m_table, "init", Qt::DirectConnection, Q_RETURN_ARG(bool, ok));
    if (ok && moreOk) {
        QMetaObject::invokeMethod(m_table, "start");
    } else {
        qmlInfo(this) << "Failed to initialize the state machine.";
    }
}

QQmlListProperty<QObject> StateMachine::states()
{
    return QQmlListProperty<QObject>(this, &m_children, append, count, at, clear);
}

Scxml::StateTable *StateMachine::stateMachine() const
{
    return m_table;
}

void StateMachine::setStateMachine(Scxml::StateTable *table)
{
    qDebug()<<"setting state machine to"<<table;
    if (m_table == Q_NULLPTR && table != Q_NULLPTR) {
        m_table = table;
    } else if (m_table) {
        qmlInfo(this) << "Can set the table only once";
    }
}

QUrl StateMachine::filename()
{
    return m_filename;
}

void StateMachine::setFilename(const QUrl &filename)
{
    QUrl oldFilename = m_filename;
    if (m_table) {
        delete m_table;
        m_table = Q_NULLPTR;
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

bool StateMachine::parse(const QUrl &filename)
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
    QXmlStreamReader xmlReader(&buf);
    Scxml::ScxmlParser parser(&xmlReader);
    parser.parse();
    setStateMachine(parser.instantiateStateMachine());

    if (parser.state() != Scxml::ScxmlParser::FinishedParsing || m_table == Q_NULLPTR) {
        qmlInfo(this) << QStringLiteral("Something went wrong while parsing '%1':").arg(filename.fileName()) << endl;
        foreach (const Scxml::ScxmlParser::ErrorMessage &msg, parser.errors()) {
            qmlInfo(this) << msg.fileName << QStringLiteral(":") << msg.line
                          << QStringLiteral(":") << msg.column
                          << QStringLiteral(": ") << msg.severityString()
                          << QStringLiteral(": ") << msg.msg;
        }

        return false;
    }

    return true;
}
