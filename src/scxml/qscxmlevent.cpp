/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscxmlexecutablecontent_p.h"
#include "qscxmlevent_p.h"
#include "qscxmlstatemachine_p.h"

QT_USE_NAMESPACE

using namespace QScxmlExecutableContent;

QEvent::Type QScxmlEvent::scxmlEventType = (QEvent::Type) QEvent::registerEventType();
QEvent::Type QScxmlEvent::ignoreEventType = (QEvent::Type) QEvent::registerEventType();

QAtomicInt QScxmlEventBuilder::idCounter = QAtomicInt(0);

QScxmlEvent *QScxmlEventBuilder::buildEvent()
{
    auto dataModel = stateMachine ? stateMachine->dataModel() : Q_NULLPTR;
    auto tableData = stateMachine ? stateMachine->tableData() : Q_NULLPTR;

    QString eventName = event;
    bool ok = true;
    if (eventexpr != NoEvaluator) {
        eventName = dataModel->evaluateToString(eventexpr, &ok);
        ok = true; // ignore failure.
    }

    QVariant data;
    if ((!params || params->count == 0) && (!namelist || namelist->count == 0)) {
        if (contentExpr == NoEvaluator) {
            data = contents;
        } else {
            data = dataModel->evaluateToString(contentExpr, &ok);
        }
        if (!ok) {
            // expr evaluation failure results in the data property of the event being set to null. See e.g. test528.
            data = QVariant(QMetaType::VoidStar, 0);
        }
    } else {
        QVariantMap keyValues;
        if (evaluate(params, stateMachine, keyValues)) {
            if (namelist) {
                for (qint32 i = 0; i < namelist->count; ++i) {
                    QString name = tableData->string(namelist->const_data()[i]);
                    keyValues.insert(name, dataModel->property(name));
                }
            }
            data = keyValues;
        } else {
            // If the evaluation of the <param> tags fails, set _event.data to an empty string.
            // See test343.
            data = QVariant(QMetaType::VoidStar, 0);
        }
    }

    QString sendid = id;
    if (!idLocation.isEmpty()) {
        sendid = generateId();
        ok = stateMachine->dataModel()->setProperty(idLocation, sendid, tableData->string(instructionLocation));
        if (!ok)
            return Q_NULLPTR;
    }

    QString origin = target;
    if (targetexpr != NoEvaluator) {
        origin = dataModel->evaluateToString(targetexpr, &ok);
        if (!ok)
            return Q_NULLPTR;
    }
    if (origin.isEmpty()) {
        if (eventType == QScxmlEvent::ExternalEvent) {
            origin = QStringLiteral("#_internal");
        }
    } else if (origin == QStringLiteral("#_parent")) {
        // allow sending messages to the parent, independently of whether we're invoked or not.
    } else if (!origin.startsWith(QLatin1Char('#'))) {
        // [6.2.4] and test194.
        submitError(QStringLiteral("error.execution"),
                    QStringLiteral("Error in %1: %2 is not a legal target")
                    .arg(tableData->string(instructionLocation), origin),
                    sendid);
        return Q_NULLPTR;
    } else if (!stateMachine->isDispatchableTarget(origin)) {
        // [6.2.4] and test521.
        submitError(QStringLiteral("error.communication"),
                    QStringLiteral("Error in %1: cannot dispatch to target '%2'")
                    .arg(tableData->string(instructionLocation), origin),
                    sendid);
        return Q_NULLPTR;
    }

    QString origintype = type;
    if (origintype.isEmpty()) {
        // [6.2.5] and test198
        origintype = QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor");
    }
    if (typeexpr != NoEvaluator) {
        origintype = dataModel->evaluateToString(typeexpr, &ok);
        if (!ok)
            return Q_NULLPTR;
    }
    if (!origintype.isEmpty()
            && origintype != QStringLiteral("qt:signal")
            && origintype != QStringLiteral("http://www.w3.org/TR/scxml/#SCXMLEventProcessor")) {
        // [6.2.5] and test199
        submitError(QStringLiteral("error.execution"),
                    QStringLiteral("Error in %1: %2 is not a valid type")
                    .arg(tableData->string(instructionLocation), origintype),
                    sendid);
        return Q_NULLPTR;
    }

    QString invokeid;
    if (stateMachine && stateMachine->isInvoked()) {
        invokeid = stateMachine->sessionId();
    }

    QScxmlEvent *event = new QScxmlEvent;
    event->setName(eventName);
    event->setEventType(eventType);
    event->setData(data);
    event->setSendId(sendid);
    event->setOrigin(origin);
    event->setOriginType(origintype);
    event->setInvokeId(invokeid);
    return event;
}

QScxmlEvent *QScxmlEventBuilder::errorEvent(QScxmlStateMachine *stateMachine, const QString &name,
                                            const QString &message, const QString &sendid)
{
    QScxmlEventBuilder event;
    event.stateMachine = stateMachine;
    event.event = name;
    event.eventType = QScxmlEvent::PlatformEvent; // Errors are platform events. See e.g. test331.
    // _event.data == null, see test528
    event.id = sendid;
    auto error = event();
    error->setErrorMessage(message);
    return error;
}

bool QScxmlEventBuilder::evaluate(const Param &param, QScxmlStateMachine *stateMachine,
                                  QVariantMap &keyValues)
{
    auto dataModel = stateMachine->dataModel();
    auto tableData = stateMachine->tableData();
    if (param.expr != NoEvaluator) {
        bool success = false;
        auto v = dataModel->evaluateToVariant(param.expr, &success);
        keyValues.insert(tableData->string(param.name), v);
        return success;
    }

    QString loc;
    if (param.location != QScxmlExecutableContent::NoString) {
        loc = tableData->string(param.location);
    }

    if (loc.isEmpty()) {
        return false;
    }

    if (dataModel->hasProperty(loc)) {
        keyValues.insert(tableData->string(param.name), dataModel->property(loc));
        return true;
    } else {
        submitError(QStringLiteral("error.execution"),
                    QStringLiteral("Error in <param>: %1 is not a valid location")
                    .arg(loc));
        return false;
    }
}

bool QScxmlEventBuilder::evaluate(const QScxmlExecutableContent::Array<Param> *params, QScxmlStateMachine *stateMachine, QVariantMap &keyValues)
{
    if (!params)
        return true;

    auto paramPtr = params->const_data();
    for (qint32 i = 0; i != params->count; ++i, ++paramPtr) {
        if (!evaluate(*paramPtr, stateMachine, keyValues))
            return false;
    }

    return true;
}

void QScxmlEventBuilder::submitError(const QString &type, const QString &msg, const QString &sendid)
{
    QScxmlStateMachinePrivate::get(stateMachine)->submitError(type, msg, sendid);
}

/*!
 * \class QScxmlEvent
 * \brief Event for an QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * See section 5.10.1 "The Internal Structure of Events" in the Scxml specification for more detail.
 *
 * \sa QScxmlStateMachine
 */

/*!
    \enum QScxmlEvent::EventType

    This enum specifies type of event.

    \value PlatformEvent is an event generated internally by the state machine. An example of these
           are errors.
    \value InternalEvent is an event generated by a <raise> tag.
    \value ExternalEvent is an event generated by a <send> tag.
 */

/*!
 * \brief Creates a new external QScxmlEvent.
 */
QScxmlEvent::QScxmlEvent()
    : QEvent(scxmlEventType), d(new QScxmlEventPrivate)
{ }

/*!
 * \brief Destroys a QScxmlEvent.
 */
QScxmlEvent::~QScxmlEvent()
{
    delete d;
}

/*!
 * \return the event type.
 */
QString QScxmlEvent::scxmlType() const
{
    switch (d->eventType) {
    case PlatformEvent:
        return QLatin1String("platform");
    case InternalEvent:
        return QLatin1String("internal");
    case ExternalEvent:
        break;
    }
    return QLatin1String("external");
}

/*!
 * \brief clears the contents of the event.
 */
void QScxmlEvent::clear()
{
    *d = QScxmlEventPrivate();
}

/*!
 * \brief Copies a QScxmlEvent.
 */
QScxmlEvent &QScxmlEvent::operator=(const QScxmlEvent &other)
{
    QEvent::operator=(other);
    *d = *other.d;
    return *this;
}

/*!
 * \brief Copies a QScxmlEvent.
 */
QScxmlEvent::QScxmlEvent(const QScxmlEvent &other)
    : QEvent(other), d(new QScxmlEventPrivate(*other.d))
{
}

/*!
 * \brief The name of the event, used in the event attribute in a <transition>
 * \return the name of the event.
 */
QString QScxmlEvent::name() const
{
    return d->name;
}

/*!
 * \brief Sets the name of the event.
 */
void QScxmlEvent::setName(const QString &name)
{
    d->name = name;
}

/*!
 * \return The id of the event, when specified in Scxml. It is used by <cancel> to identify the
 *         event that has to be canceled. Note that the system will generate a unique ID when the
 *         idlocation attribute is used in <send>
 */
QString QScxmlEvent::sendId() const
{
    return d->sendid;
}

/*!
 * \brief sets the id for this event.
 */
void QScxmlEvent::setSendId(const QString &sendid)
{
    d->sendid = sendid;
}

/*!
 * \brief This is a URI, equivalent to the 'target' attribute on the <send> element.
 */
QString QScxmlEvent::origin() const
{
    return d->origin;
}

void QScxmlEvent::setOrigin(const QString &origin)
{
    d->origin = origin;
}

/*!
 * \brief This is equivalent to the 'type' field on the <send> element.
 */
QString QScxmlEvent::originType() const
{
    return d->originType;
}

/*!
 * \brief Sets the \a origintype of a QScxmlEvent.
 *
 * \sa QScxmlEvent::originType
 */
void QScxmlEvent::setOriginType(const QString &origintype)
{
    d->originType = origintype;
}

/*!
 * \brief If this event is generated by an invoked state machine, it is set to the id of the invoke.
 *        Otherwise it will be empty.
 */
QString QScxmlEvent::invokeId() const
{
    return d->invokeId;
}

/*!
 * \brief Sets the \a invokeid of an event.
 * \sa QScxmlEvent::invokeId
 */
void QScxmlEvent::setInvokeId(const QString &invokeid)
{
    d->invokeId = invokeid;
}

/*!
 * \brief The delay in miliseconds after which this event is to be delivered after processing the
 *        <send>.
 */
int QScxmlEvent::delay() const
{
    return d->delayInMiliSecs;
}

/*!
 * \brief Sets the delay in miliseconds.
 * \sa QScxmlEvent::delay
 */
void QScxmlEvent::setDelay(int delayInMiliSecs)
{
    d->delayInMiliSecs = delayInMiliSecs;
}

/*!
 * \return The type of this event.
 * \sa QScxmlEvent::EventType
 */
QScxmlEvent::EventType QScxmlEvent::eventType() const
{
    return d->eventType;
}

/*!
 * \brief Sets the event type.
 * \sa QScxmlEvent::eventType QScxmlEvent::EventType
 */
void QScxmlEvent::setEventType(const EventType &type)
{
    d->eventType = type;
}

/*!
 * \brief Any data included by the sender.
 *
 * When <param> tags are used in the <send>, the data will contain a QVariantMap where the key is
 * the name attribute, and the value is taken from the expr attribute or the location attribute.
 * When a <content> tag is used, the data will contain a single item with the content's expr or
 * child data.
 */
QVariant QScxmlEvent::data() const
{
    if (isErrorEvent())
        return QVariant();
    return d->data;
}

/*!
 * \brief Sets the payload data.
 * \sa QScxmlEvent::data
 */
void QScxmlEvent::setData(const QVariant &data)
{
    if (!isErrorEvent())
        d->data = data;
}

/*!
 * \return true when this is an error event, false otherwise.
 */
bool QScxmlEvent::isErrorEvent() const
{
    return eventType() == PlatformEvent && name().startsWith(QStringLiteral("error."));
}

/*!
 * \return If this is an error event, it returns the error message. Otherwise it will return an
 *         empty QString.
 */
QString QScxmlEvent::errorMessage() const
{
    if (!isErrorEvent())
        return QString();
    return d->data.toString();
}

/*!
 * \param message If this is an error event, the \a message will be set as the error message.
 */
void QScxmlEvent::setErrorMessage(const QString &message)
{
    if (isErrorEvent())
        d->data = message;
}

void QScxmlEvent::makeIgnorable()
{
    t = ignoreEventType;
}
