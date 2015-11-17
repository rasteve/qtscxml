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

#ifndef SCXMLEVENT_H
#define SCXMLEVENT_H

#include <QtScxml/qscxmlglobals.h>

#include <QEvent>
#include <QStringList>
#include <QVariantList>

QT_BEGIN_NAMESPACE

namespace QScxmlInternal {
class WrappedQStateMachine;
}

class QScxmlEventPrivate;

class Q_SCXML_EXPORT QScxmlEvent: public QEvent
{
public:
    QScxmlEvent();
    ~QScxmlEvent();

    QScxmlEvent &operator=(const QScxmlEvent &other);
    QScxmlEvent(const QScxmlEvent &other);

    enum EventType {
        PlatformEvent,
        InternalEvent,
        ExternalEvent
    };

    QByteArray name() const;
    void setName(const QByteArray &name);

    EventType eventType() const;
    void setEventType(const EventType &type);

    QString scxmlType() const;

    QByteArray sendId() const;
    void setSendId(const QByteArray &sendId);

    QString origin() const;
    void setOrigin(const QString &origin);

    QString originType() const;
    void setOriginType(const QString &originType);

    QString invokeId() const;
    void setInvokeId(const QString &invokeId);

    int delay() const;
    void setDelay(int delayInMiliSecs);

    void clear();

    QVariant data() const;
    void setData(const QVariant &data);

protected:
    friend class QScxmlInternal::WrappedQStateMachine;
    static QEvent::Type scxmlEventType;
    static QEvent::Type ignoreEventType;
    void makeIgnorable();

private:
    QScxmlEventPrivate *d;

};

QT_END_NAMESPACE

#endif // SCXMLEVENT_H
