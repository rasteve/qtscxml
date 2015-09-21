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

#include "qscxmlinvokableservice.h"
#include "qscxmlstatemachine_p.h"

QT_BEGIN_NAMESPACE

class QScxmlInvokableService::Data
{
public:
    Data(const QString &id, const QVariantMap &data, bool autoforward,
         QScxmlExecutableContent::ContainerId finalize, QScxmlStateMachine *parent)
        : id(id)
        , data(data)
        , autoforward(autoforward)
        , parent(parent)
        , finalize(finalize)
    {}

    QString id;
    QVariantMap data;
    bool autoforward;
    QScxmlStateMachine *parent;
    QScxmlExecutableContent::ContainerId finalize;
};

QScxmlInvokableService::QScxmlInvokableService(const QString &id,
                                             const QVariantMap &data,
                                             bool autoforward,
                                             QScxmlExecutableContent::ContainerId finalize,
                                             QScxmlStateMachine *parent)
    : d(new Data(id, data, autoforward, finalize, parent))
{}

QScxmlInvokableService::~QScxmlInvokableService()
{
    delete d;
}

QString QScxmlInvokableService::id() const
{
    return d->id;
}

bool QScxmlInvokableService::autoforward() const
{
    return d->autoforward;
}

QVariantMap QScxmlInvokableService::data() const
{
    return d->data;
}

QScxmlStateMachine *QScxmlInvokableService::parent() const
{
    return d->parent;
}

void QScxmlInvokableService::finalize()
{
    auto smp = QScxmlStateMachinePrivate::get(parent());
    smp->m_executionEngine->execute(d->finalize);
}

class QScxmlInvokableServiceFactory::Data
{
public:
    Data(QScxmlExecutableContent::StringId invokeLocation, QScxmlExecutableContent::StringId id,
         QScxmlExecutableContent::StringId idPrefix, QScxmlExecutableContent::StringId idlocation,
         const QVector<QScxmlExecutableContent::StringId> &namelist, bool autoforward,
         const QVector<QScxmlInvokableServiceFactory::Param> &params,
         QScxmlExecutableContent::ContainerId finalize)
        : invokeLocation(invokeLocation)
        , id(id)
        , idPrefix(idPrefix)
        , idlocation(idlocation)
        , namelist(namelist)
        , autoforward(autoforward)
        , params(params)
        , finalize(finalize)
    {}

    QScxmlExecutableContent::StringId invokeLocation;
    QScxmlExecutableContent::StringId id;
    QScxmlExecutableContent::StringId idPrefix;
    QScxmlExecutableContent::StringId idlocation;
    QVector<QScxmlExecutableContent::StringId> namelist;
    bool autoforward;
    QVector<QScxmlInvokableServiceFactory::Param> params;
    QScxmlExecutableContent::ContainerId finalize;

};

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,  QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix, QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist, bool autoforward,
        const QVector<Param> &params, QScxmlExecutableContent::ContainerId finalize)
    : d(new Data(invokeLocation, id, idPrefix, idlocation, namelist, autoforward, params, finalize))
{}

QScxmlInvokableServiceFactory::~QScxmlInvokableServiceFactory()
{
    delete d;
}

QString QScxmlInvokableServiceFactory::calculateId(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto stateMachine = parent->tableData();

    if (d->id != QScxmlExecutableContent::NoString) {
        return stateMachine->string(d->id);
    }

    QString id = QScxmlStateMachine::generateSessionId(stateMachine->string(d->idPrefix));

    if (d->idlocation != QScxmlExecutableContent::NoString) {
        auto idloc = stateMachine->string(d->idlocation);
        auto ctxt = stateMachine->string(d->invokeLocation);
        parent->dataModel()->setProperty(idloc, id, ctxt, ok);
        if (!*ok)
            return QString();
    }

    return id;
}

QVariantMap QScxmlInvokableServiceFactory::calculateData(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);

    QVariantMap result;
    auto dataModel = parent->dataModel();
    auto tableData = parent->tableData();

    foreach (const Param &param, d->params) {
        auto name = tableData->string(param.name);

        if (param.expr != QScxmlExecutableContent::NoEvaluator) {
            *ok = false;
            auto v = dataModel->evaluateToVariant(param.expr, ok);
            if (!*ok)
                return QVariantMap();
            result.insert(name, v);
        } else {
            QString loc;
            if (param.location != QScxmlExecutableContent::NoString) {
                loc = tableData->string(param.location);
            }

            if (loc.isEmpty()) {
                // TODO: error message?
                *ok = false;
                return QVariantMap();
            }

            auto v = dataModel->property(loc);
            result.insert(name, v);
        }
    }

    foreach (QScxmlExecutableContent::StringId locid, d->namelist) {
        QString loc;
        if (locid != QScxmlExecutableContent::NoString) {
            loc = tableData->string(locid);
        }
        if (loc.isEmpty()) {
            // TODO: error message?
            *ok = false;
            return QVariantMap();
        }
        if (dataModel->hasProperty(loc)) {
            auto v = dataModel->property(loc);
            result.insert(loc, v);
        } else {
            *ok = false;
            return QVariantMap();
        }
    }

    return result;
}

bool QScxmlInvokableServiceFactory::autoforward() const
{
    return d->autoforward;
}

QScxmlExecutableContent::ContainerId QScxmlInvokableServiceFactory::finalizeContent() const
{
    return d->finalize;
}

namespace {
class InvokableScxml: public QScxmlInvokableService
{
public:
    InvokableScxml(QScxmlStateMachine *stateMachine, const QString &id, const QVariantMap &data,
                   bool autoforward, QScxmlExecutableContent::ContainerId finalize, QScxmlStateMachine *parent)
        : QScxmlInvokableService(id, data, autoforward, finalize, parent)
        , m_stateMachine(stateMachine)
    {
        stateMachine->setSessionId(id);
        stateMachine->setParentStateMachine(parent);
        stateMachine->init(data);
        stateMachine->start();
    }

    ~InvokableScxml()
    { delete m_stateMachine; }

    void submitEvent(QScxmlEvent *event) Q_DECL_OVERRIDE
    {
        m_stateMachine->submitEvent(event);
    }

private:
    QScxmlStateMachine *m_stateMachine;
};
} // anonymous namespace

QScxmlInvokableScxmlServiceFactory::QScxmlInvokableScxmlServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,
        QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix,
        QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist,
        bool doAutoforward,
        const QVector<QScxmlInvokableServiceFactory::Param> &params,
        QScxmlExecutableContent::ContainerId finalize)
    : QScxmlInvokableServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist,
                                   doAutoforward, params, finalize)
{}

QScxmlInvokableService *QScxmlInvokableScxmlServiceFactory::finishInvoke(QScxmlStateMachine *child, QScxmlStateMachine *parent)
{
    bool ok = false;
    auto id = calculateId(parent, &ok);
    if (!ok)
        return Q_NULLPTR;
    auto data = calculateData(parent, &ok);
    if (!ok)
        return Q_NULLPTR;
    child->setIsInvoked(true);
    return new InvokableScxml(child, id, data, autoforward(), finalizeContent(), parent);
}

QT_END_NAMESPACE
