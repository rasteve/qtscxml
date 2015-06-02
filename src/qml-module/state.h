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

#ifndef STATE_H
#define STATE_H

#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>

namespace Scxml {
class ScxmlState;
}

QT_BEGIN_NAMESPACE

class StateMachine;
class State: public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(QString scxmlName READ scxmlName WRITE setScxmlName NOTIFY scxmlNameChanged)

public:
    explicit State(StateMachine *parent = 0);

    void classBegin() {}
    void componentComplete();

    bool isActive() const;

    QString scxmlName() const;
    void setScxmlName(const QString &scxmlName);

Q_SIGNALS:
    void activeChanged(bool active);
    void scxmlNameChanged();
    void didEnter();
    void willExit();

private:
    QString m_scxmlName;
    Scxml::ScxmlState *m_state = nullptr;
};

QT_END_NAMESPACE

#endif
