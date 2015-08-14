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

#ifndef SCXMLEVENT_H
#define SCXMLEVENT_H

#include <QtScxml/scxmlglobals.h>

#include <QEvent>
#include <QStringList>
#include <QVariantList>

QT_BEGIN_NAMESPACE

namespace Scxml {

class SCXML_EXPORT ScxmlEvent: public QEvent {
public:
    static QEvent::Type scxmlEventType;
    enum EventType { Platform, Internal, External };

    ScxmlEvent(const QByteArray &name = QByteArray(), EventType eventType = External,
               const QVariantList &dataValues = QVariantList(), const QStringList &dataNames = QStringList(),
               const QByteArray &sendid = QByteArray (),
               const QString &origin = QString (), const QString &origintype = QString (),
               const QByteArray &invokeid = QByteArray());

    QByteArray name() const { return m_name; }
    EventType eventType() const { return m_type; }
    QString scxmlType() const;
    QByteArray sendid() const { return m_sendid; }
    QString origin() const { return m_origin; }
    QString origintype() const { return m_origintype; }
    QByteArray invokeid() const { return m_invokeid; }
    QVariantList dataValues() const { return m_dataValues; }
    QStringList dataNames() const { return m_dataNames; }
    void reset(const QByteArray &name, EventType eventType = External,
               QVariantList dataValues = QVariantList(), const QByteArray &sendid = QByteArray(),
               const QString &origin = QString(), const QString &origintype = QString(),
               const QByteArray &invokeid = QByteArray());
    void clear();

    QVariant data() const;

private:
    QByteArray m_name;
    EventType m_type;
    QVariantList m_dataValues; // extra data
    QStringList m_dataNames; // extra data
    QByteArray m_sendid; // if set, or id of <send> if failure
    QString m_origin; // uri to answer by setting the target of send, empty for internal and platform events
    QString m_origintype; // type to answer by setting the type of send, empty for internal and platform events
    QByteArray m_invokeid; // id of the invocation that triggered the child process if this was invoked
};

} // namespace Scxml

QT_END_NAMESPACE

#endif // SCXMLEVENT_H
