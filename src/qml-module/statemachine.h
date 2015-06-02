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

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <QVector>
#include <QQmlParserStatus>
#include <QQmlListProperty>
#include <QScxmlLib/scxmlstatetable.h>

QT_BEGIN_NAMESPACE

class State;
class QQmlOpenMetaObject;
class StateMachine: public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> states READ states NOTIFY statesChanged DESIGNABLE false)
    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(Scxml::StateTable* stateMachine READ stateMachine WRITE setStateMachine)

    Q_CLASSINFO("DefaultProperty", "states")

public:
    typedef QVector<State *> States;
    explicit StateMachine(QObject *parent = 0);

    void classBegin() {}
    void componentComplete();
    QQmlListProperty<QObject> states();

    Scxml::StateTable *stateMachine() const;
    void setStateMachine(Scxml::StateTable *stateMachine);

    QString filename();
    void setFilename(const QString filename);

Q_SIGNALS:
    void statesChanged();
    void filenameChanged();

private:
    bool parse(const QString &filename);

private:
    QString m_filename;
    States m_states;
    Scxml::StateTable *m_table = nullptr;
};

QT_END_NAMESPACE

#endif
