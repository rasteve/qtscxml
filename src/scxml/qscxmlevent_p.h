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

#ifndef SCXMLEVENT_P_H
#define SCXMLEVENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtScxml/qscxmlevent.h>
#include <QtScxml/private/qscxmlexecutablecontent_p.h>

#ifndef BUILD_QSCXMLC
#include <QtScxml/qscxmlstatemachine.h>
#endif

#include <QAtomicInt>

QT_BEGIN_NAMESPACE

#ifndef BUILD_QSCXMLC
class QScxmlEventBuilder
{
    QScxmlStateMachine* stateMachine;
    QScxmlExecutableContent::StringId instructionLocation;
    QByteArray event;
    QScxmlExecutableContent::EvaluatorId eventexpr;
    QString contents;
    QScxmlExecutableContent::EvaluatorId contentExpr;
    const QScxmlExecutableContent::Array<QScxmlExecutableContent::Param> *params;
    QScxmlEvent::EventType eventType;
    QByteArray id;
    QString idLocation;
    QString target;
    QScxmlExecutableContent::EvaluatorId targetexpr;
    QString type;
    QScxmlExecutableContent::EvaluatorId typeexpr;
    const QScxmlExecutableContent::Array<QScxmlExecutableContent::StringId> *namelist;

    static QAtomicInt idCounter;
    QByteArray generateId() const
    {
        QByteArray id = QByteArray::number(++idCounter);
        id.prepend("id-");
        return id;
    }

    QScxmlEventBuilder()
    { init(); }

    void init() // Because stupid VS2012 can't cope with non-static field initializers.
    {
        stateMachine = Q_NULLPTR;
        eventexpr = QScxmlExecutableContent::NoEvaluator;
        contentExpr = QScxmlExecutableContent::NoEvaluator;
        params = Q_NULLPTR;
        eventType = QScxmlEvent::ExternalEvent;
        targetexpr = QScxmlExecutableContent::NoEvaluator;
        typeexpr = QScxmlExecutableContent::NoEvaluator;
        namelist = Q_NULLPTR;
    }

public:
    QScxmlEventBuilder(QScxmlStateMachine *stateMachine, const QString &eventName, const QScxmlExecutableContent::DoneData *doneData)
    {
        init();
        this->stateMachine = stateMachine;
        Q_ASSERT(doneData);
        instructionLocation = doneData->location;
        event = eventName.toUtf8();
        contents = stateMachine->tableData()->string(doneData->contents);
        contentExpr = doneData->expr;
        params = &doneData->params;
    }

    QScxmlEventBuilder(QScxmlStateMachine *stateMachine, QScxmlExecutableContent::Send &send)
    {
        init();
        this->stateMachine = stateMachine;
        instructionLocation = send.instructionLocation;
        event = stateMachine->tableData()->byteArray(send.event);
        eventexpr = send.eventexpr;
        contents = stateMachine->tableData()->string(send.content);
        params = send.params();
        id = stateMachine->tableData()->byteArray(send.id);
        idLocation = stateMachine->tableData()->string(send.idLocation);
        target = stateMachine->tableData()->string(send.target);
        targetexpr = send.targetexpr;
        type = stateMachine->tableData()->string(send.type);
        typeexpr = send.typeexpr;
        namelist = &send.namelist;
    }

    QScxmlEvent *operator()() { return buildEvent(); }

    QScxmlEvent *buildEvent();

    static QScxmlEvent *errorEvent(QScxmlStateMachine *stateMachine, const QByteArray &name,
                                   const QString &message, const QByteArray &sendid);

    static bool evaluate(const QScxmlExecutableContent::Param &param, QScxmlStateMachine *stateMachine,
                         QVariantMap &keyValues);

    static bool evaluate(const QScxmlExecutableContent::Array<QScxmlExecutableContent::Param> *params,
                         QScxmlStateMachine *stateMachine, QVariantMap &keyValues);
};
#endif // BUILD_QSCXMLC

class QScxmlEventPrivate
{
public:
    QScxmlEventPrivate()
        : eventType(QScxmlEvent::ExternalEvent)
        , delayInMiliSecs(0)
    {}

    QByteArray name;
    QScxmlEvent::EventType eventType;
    QVariant data; // extra data
    QByteArray sendid; // if set, or id of <send> if failure
    QString origin; // uri to answer by setting the target of send, empty for internal and platform events
    QString originType; // type to answer by setting the type of send, empty for internal and platform events
    QString invokeId; // id of the invocation that triggered the child process if this was invoked
    int delayInMiliSecs;
};

QT_END_NAMESPACE

#endif // SCXMLEVENT_P_H

