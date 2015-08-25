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

#include "executablecontent_p.h"
#include "scxmlevent_p.h"

QT_USE_NAMESPACE

using namespace Scxml;

QEvent::Type QScxmlEvent::scxmlEventType = (QEvent::Type) QEvent::registerEventType();
QEvent::Type QScxmlEvent::ignoreEventType = (QEvent::Type) QEvent::registerEventType();

static bool evaluate(const ExecutableContent::Param &param, StateMachine *table, QVariantList &dataValues, QStringList &dataNames)
{
    auto dataModel = table->dataModel();
    auto tableData = table->tableData();
    if (param.expr != NoEvaluator) {
        bool success = false;
        auto v = dataModel->evaluateToVariant(param.expr, &success);
        dataValues.append(v);
        dataNames.append(tableData->string(param.name));
        return success;
    }

    QString loc;
    if (param.location != ExecutableContent::NoString) {
        loc = tableData->string(param.location);
    }

    if (loc.isEmpty()) {
        return false;
    }

    if (dataModel->hasProperty(loc)) {
        dataValues.append(dataModel->property(loc));
        dataNames.append(tableData->string(param.name));
        return true;
    } else {
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in <param>: %1 is not a valid location")
                           .arg(loc),
                           /*sendid =*/ QByteArray());
        return false;
    }
}

static bool evaluate(const ExecutableContent::Array<ExecutableContent::Param> *params, StateMachine *table, QVariantList &dataValues, QStringList &dataNames)
{
    if (!params)
        return true;

    auto paramPtr = params->const_data();
    for (qint32 i = 0; i != params->count; ++i, ++paramPtr) {
        if (!evaluate(*paramPtr, table, dataValues, dataNames))
            return false;
    }

    return true;
}

QAtomicInt EventBuilder::idCounter = QAtomicInt(0);

QScxmlEvent *EventBuilder::buildEvent()
{
    auto dataModel = table ? table->dataModel() : Q_NULLPTR;
    auto tableData = table ? table->tableData() : Q_NULLPTR;

    QByteArray eventName = event;
    bool ok = true;
    if (eventexpr != NoEvaluator) {
        eventName = dataModel->evaluateToString(eventexpr, &ok).toUtf8();
        ok = true; // ignore failure.
    }

    QVariantList dataValues;
    QStringList dataNames;
    if ((!params || params->count == 0) && (!namelist || namelist->count == 0)) {
        QVariant data;
        if (contentExpr != NoEvaluator) {
            data = dataModel->evaluateToString(contentExpr, &ok);
        } else {
            data = contents;
        }
        if (ok) {
            dataValues.append(data);
        } else {
            // expr evaluation failure results in the data property of the event being set to null. See e.g. test528.
            dataValues = QVariantList() << QVariant(QMetaType::VoidStar, 0);
        }
    } else {
        if (evaluate(params, table, dataValues, dataNames)) {
            if (namelist) {
                for (qint32 i = 0; i < namelist->count; ++i) {
                    QString name = tableData->string(namelist->const_data()[i]);
                    dataNames << name;
                    dataValues << dataModel->property(name);
                }
            }
        } else {
            // If the evaluation of the <param> tags fails, set _event.data to an empty string.
            // See test343.
            dataValues = QVariantList() << QVariant(QMetaType::VoidStar, 0);
            dataNames.clear();
        }
    }

    QByteArray sendid = id;
    if (!idLocation.isEmpty()) {
        sendid = generateId();
        table->dataModel()->setStringProperty(idLocation, QString::fromUtf8(sendid), tableData->string(instructionLocation), &ok);
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
    } else if (!table->isLegalTarget(origin)) {
        // [6.2.4] and test194.
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in %1: %2 is not a legal target")
                           .arg(tableData->string(instructionLocation), origin),
                           sendid);
        return Q_NULLPTR;
    } else if (!table->isDispatchableTarget(origin)) {
        // [6.2.4] and test521.
        table->submitError(QByteArray("error.communication"),
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
        table->submitError(QByteArray("error.execution"),
                           QStringLiteral("Error in %1: %2 is not a valid type")
                           .arg(tableData->string(instructionLocation), origintype),
                           sendid);
        return Q_NULLPTR;
    }

    QScxmlEvent *event = new QScxmlEvent;
    event->setName(eventName);
    event->setEventType(eventType);
    event->setDataValues(dataValues);
    event->setDataNames(dataNames);
    event->setSendId(sendid);
    event->setOrigin(origin);
    event->setOriginType(origintype);
    return event;
}

QScxmlEvent::QScxmlEvent()
    : QEvent(scxmlEventType), d(new QScxmlEventPrivate)
{ }

QScxmlEvent::~QScxmlEvent()
{
    delete d;
}

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

void QScxmlEvent::reset(const QByteArray &name, QScxmlEvent::EventType eventType, QVariantList dataValues,
                       const QByteArray &sendid, const QString &origin,
                       const QString &origintype, const QByteArray &invokeid)
{
    d->name = name;
    d->eventType = eventType;
    d->sendid = sendid;
    d->origin = origin;
    d->originType = origintype;
    d->invokeId = invokeid;
    d->dataValues = dataValues;
}

void QScxmlEvent::clear()
{
    d->name = QByteArray();
    d->eventType = ExternalEvent;
    d->sendid = QByteArray();
    d->origin = QString();
    d->originType = QString();
    d->invokeId = QByteArray();
    d->dataValues = QVariantList();
}

QScxmlEvent &QScxmlEvent::operator=(const QScxmlEvent &other)
{
    QEvent::operator=(other);
    *d = *other.d;
    return *this;
}

QScxmlEvent::QScxmlEvent(const QScxmlEvent &other)
    : QEvent(other), d(new QScxmlEventPrivate(*other.d))
{
}

QByteArray QScxmlEvent::name() const
{
    return d->name;
}

void QScxmlEvent::setName(const QByteArray &name)
{
    d->name = name;
}

QByteArray QScxmlEvent::sendId() const
{
    return d->sendid;
}

void QScxmlEvent::setSendId(const QByteArray &sendid)
{
    d->sendid = sendid;
}

QString QScxmlEvent::origin() const
{
    return d->origin;
}

void QScxmlEvent::setOrigin(const QString &origin)
{
    d->origin = origin;
}

QString QScxmlEvent::originType() const
{
    return d->originType;
}

void QScxmlEvent::setOriginType(const QString &origintype)
{
    d->originType = origintype;
}

QByteArray QScxmlEvent::invokeId() const
{
    return d->invokeId;
}

void QScxmlEvent::setInvokeId(const QByteArray &invokeid)
{
    d->invokeId = invokeid;
}

QVariantList QScxmlEvent::dataValues() const
{
    return d->dataValues;
}

void QScxmlEvent::setDataValues(const QVariantList &dataValues)
{
    d->dataValues = dataValues;
}

QStringList QScxmlEvent::dataNames() const
{
    return d->dataNames;
}

void QScxmlEvent::setDataNames(const QStringList &dataNames)
{
    d->dataNames = dataNames;
}

QScxmlEvent::EventType QScxmlEvent::eventType() const
{
    return d->eventType;
}

void QScxmlEvent::setEventType(const EventType &type)
{
    d->eventType = type;
}

