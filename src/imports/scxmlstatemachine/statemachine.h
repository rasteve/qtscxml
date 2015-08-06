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

#include <QUrl>
#include <QVector>
#include <QQmlParserStatus>
#include <QQmlListProperty>
#include <QScxml/scxmlstatemachine.h>
#include <QScxml/ecmascriptdatamodel.h>

QT_BEGIN_NAMESPACE

class State;
class QQmlOpenMetaObject;
class StateMachine: public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> states READ states NOTIFY statesChanged DESIGNABLE false)
    Q_PROPERTY(QUrl filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(Scxml::StateMachine* stateMachine READ stateMachine WRITE setStateMachine DESIGNABLE false)

    Q_CLASSINFO("DefaultProperty", "states")

public:
    typedef QVector<QObject *> Kids;
    explicit StateMachine(QObject *parent = 0);

    void classBegin() {}
    void componentComplete();
    QQmlListProperty<QObject> states();

    Scxml::StateMachine *stateMachine() const;
    void setStateMachine(Scxml::StateMachine *stateMachine);

    QUrl filename();
    void setFilename(const QUrl &filename);

Q_SIGNALS:
    void statesChanged();
    void filenameChanged();

private:
    bool parse(const QUrl &filename);

private:
    QUrl m_filename;
    Kids m_children;
    Scxml::StateMachine *m_table;
    QScopedPointer<Scxml::EcmaScriptDataModel> m_dataModel;
};

QT_END_NAMESPACE

#endif
