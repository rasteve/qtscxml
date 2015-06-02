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

#include <QScxmlLib/ecmascriptdatamodel.h>
#include <QScxmlLib/scxmlstatetable.h>
#include <QScxmlLib/scxmlparser.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QFile>

static void append(QQmlListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    if (State *state = qobject_cast<State *>(o)) {
        state->setParent(prop->object);
        static_cast<StateMachine::States *>(prop->data)->append(state);
        emit static_cast<StateMachine *>(prop->object)->statesChanged();
    } else {
        qmlInfo(prop->object) << "StateMachine can only contain State items";
    }
}

static int count(QQmlListProperty<QObject> *prop)
{
    return static_cast<StateMachine::States *>(prop->data)->count();
}

static QObject *at(QQmlListProperty<QObject> *prop, int index)
{
    return static_cast<StateMachine::States *>(prop->data)->at(index);
}

static void clear(QQmlListProperty<QObject> *prop)
{
    static_cast<StateMachine::States *>(prop->data)->clear();
    emit static_cast<StateMachine *>(prop->object)->statesChanged();
}

StateMachine::StateMachine(QObject *parent)
    : QObject(parent)
{
}

void StateMachine::componentComplete()
{
    if (m_table == nullptr) {
         qmlInfo(this) << "No state machine loaded.";
         return;
    }

    m_table->start();
}

QQmlListProperty<QObject> StateMachine::states()
{
    return QQmlListProperty<QObject>(this, &m_states, append, count, at, clear);
}

Scxml::StateTable *StateMachine::stateMachine() const
{
    return m_table;
}

void StateMachine::setStateMachine(Scxml::StateTable *table)
{
    qDebug()<<"setting state machine to"<<table;
    if (m_table == nullptr && table != nullptr) {
        m_table = table;
        m_table->init();
        QQmlContext *context = QQmlEngine::contextForObject(parent());
        if (Scxml::EcmaScriptDataModel *dataModel = m_table->dataModel()->asEcmaScriptDataModel())
            dataModel->setEngine(context->engine());
    } else if (m_table) {
        qmlInfo(this) << "Can set the table only once";
    }
}

QString StateMachine::filename()
{
    return m_filename;
}

void StateMachine::setFilename(const QString filename)
{
    QString oldFilename = m_filename;
    if (m_table) {
        delete m_table;
        m_table = nullptr;
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

bool StateMachine::parse(const QString &filename)
{
    QFile scxmlFile(filename);
    if (!scxmlFile.open(QIODevice::ReadOnly)) {
        qmlInfo(this) << QStringLiteral("ERROR: cannot open '%1' for reading!").arg(filename);
        return false;
    }

    QXmlStreamReader xmlReader(&scxmlFile);
    Scxml::ScxmlParser parser(&xmlReader);
    parser.parse();
    scxmlFile.close();
    setStateMachine(parser.table());

    if (parser.state() != Scxml::ScxmlParser::FinishedParsing || m_table == nullptr) {
        qmlInfo(this) << QStringLiteral("Something went wrong while parsing '%1':").arg(filename) << endl;
        foreach (const Scxml::ErrorMessage &msg, parser.errors()) {
            qmlInfo(this) << msg.fileName << QStringLiteral(":") << msg.line
                          << QStringLiteral(":") << msg.column
                          << QStringLiteral(": ") << msg.severityString()
                          << QStringLiteral(": ") << msg.msg;
        }

        return false;
    }

    return true;
}
