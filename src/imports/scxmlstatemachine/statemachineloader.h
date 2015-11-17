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

#ifndef STATEMACHINELOADER_H
#define STATEMACHINELOADER_H

#include <QUrl>
#include <QtScxml/qscxmlstatemachine.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

class QScxmlStateMachineLoader: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(QScxmlStateMachine* stateMachine READ stateMachine DESIGNABLE false NOTIFY stateMachineChanged)

public:
    explicit QScxmlStateMachineLoader(QObject *parent = 0);

    QScxmlStateMachine *stateMachine() const;

    QUrl filename();
    void setFilename(const QUrl &filename);

Q_SIGNALS:
    void filenameChanged();
    void stateMachineChanged();

private:
    bool parse(const QUrl &filename);

private:
    QUrl m_filename;
    QScxmlStateMachine *m_stateMachine;
};

QT_END_NAMESPACE

#endif // STATEMACHINELOADER_H
