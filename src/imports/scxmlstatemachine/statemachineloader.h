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

#ifndef STATEMACHINELOADER_H
#define STATEMACHINELOADER_H

#include <QUrl>
#include <QtScxml/scxmlstatemachine.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

class StateMachineLoader: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(Scxml::StateMachine* stateMachine READ stateMachine DESIGNABLE false NOTIFY stateMachineChanged)

public:
    explicit StateMachineLoader(QObject *parent = 0);

    Scxml::StateMachine *stateMachine() const;

    QUrl filename();
    void setFilename(const QUrl &filename);

Q_SIGNALS:
    void filenameChanged();
    void stateMachineChanged();

private:
    bool parse(const QUrl &filename);

private:
    QUrl m_filename;
    Scxml::StateMachine *m_stateMachine;
};

QT_END_NAMESPACE

#endif // STATEMACHINELOADER_H
