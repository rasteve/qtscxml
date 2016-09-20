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

#include "qscxmlglobals_p.h"
#include "qscxmlinvokableservice.h"
#include "qscxmlstatemachine_p.h"

QT_BEGIN_NAMESPACE

class QScxmlInvokableServicePrivate : public QObjectPrivate
{
public:
    QScxmlInvokableServicePrivate(QScxmlInvokableServiceFactory *factory,
                                  QScxmlStateMachine *parentStateMachine)
        : factory(factory), parentStateMachine(parentStateMachine)
    {}

    QScxmlInvokableServiceFactory *factory;
    QScxmlStateMachine *parentStateMachine;
};

static void registerMetatype()
{
    static int metaType = qRegisterMetaType<QScxmlInvokableService *>();
    Q_UNUSED(metaType);
}

QScxmlInvokableService::QScxmlInvokableService(QScxmlInvokableServiceFactory *factory,
                                               QScxmlStateMachine *parentStateMachine,
                                               QObject *parent) :
    QObject(*(new QScxmlInvokableServicePrivate(factory, parentStateMachine)), parent)
{
    registerMetatype();
}

bool QScxmlInvokableService::autoforward() const
{
    Q_D(const QScxmlInvokableService);
    return d->factory->autoforward();
}

QScxmlStateMachine *QScxmlInvokableService::parentStateMachine() const
{
    Q_D(const QScxmlInvokableService);
    return d->parentStateMachine;
}

void QScxmlInvokableService::finalize()
{
    Q_D(QScxmlInvokableService);
    QScxmlExecutableContent::ContainerId finalize = d->factory->finalizeContent();

    if (finalize != QScxmlExecutableContent::NoInstruction) {
        auto psm = parentStateMachine();
        qCDebug(qscxmlLog) << psm << "running finalize on event";
        auto smp = QScxmlStateMachinePrivate::get(psm);
        smp->m_executionEngine->execute(finalize);
    }
}

QScxmlInvokableServiceFactory *QScxmlInvokableService::factory() const
{
    Q_D(const QScxmlInvokableService);
    return d->factory;
}

QScxmlInvokableService::QScxmlInvokableService(QScxmlInvokableServicePrivate &dd, QObject *parent) :
    QObject(dd, parent)
{
    registerMetatype();
}

class QScxmlInvokableServiceFactoryPrivate
{
public:
    QScxmlInvokableServiceFactoryPrivate(QScxmlExecutableContent::StringId invokeLocation,
         QScxmlExecutableContent::EvaluatorId srcexpr,
         QScxmlExecutableContent::StringId id,
         QScxmlExecutableContent::StringId idPrefix,
         QScxmlExecutableContent::StringId idlocation,
         const QVector<QScxmlExecutableContent::StringId> &namelist,
         bool autoforward,
         const QVector<QScxmlExecutableContent::ParameterInfo> &params,
         QScxmlExecutableContent::ContainerId finalize)
        : invokeLocation(invokeLocation)
        , srcexpr(srcexpr)
        , id(id)
        , idPrefix(idPrefix)
        , idlocation(idlocation)
        , namelist(namelist)
        , params(params)
        , finalize(finalize)
        , autoforward(autoforward)
    {}

    QScxmlExecutableContent::StringId invokeLocation;
    QScxmlExecutableContent::EvaluatorId srcexpr;
    QScxmlExecutableContent::StringId id;
    QScxmlExecutableContent::StringId idPrefix;
    QScxmlExecutableContent::StringId idlocation;
    QVector<QScxmlExecutableContent::StringId> namelist;
    QVector<QScxmlExecutableContent::ParameterInfo> params;
    QScxmlExecutableContent::ContainerId finalize;
    bool autoforward;
};

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,
        QScxmlExecutableContent::EvaluatorId srcexpr,
        QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix,
        QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist,
        bool autoforward,
        const QVector<QScxmlExecutableContent::ParameterInfo> &params,
        QScxmlExecutableContent::ContainerId finalize)
    : d(new QScxmlInvokableServiceFactoryPrivate(invokeLocation,
                                                 srcexpr,
                                                 id,
                                                 idPrefix,
                                                 idlocation,
                                                 namelist,
                                                 autoforward,
                                                 params,
                                                 finalize))
{}

QScxmlInvokableServiceFactory::~QScxmlInvokableServiceFactory()
{
    delete d;
}

QString QScxmlInvokableServiceFactory::calculateSrcexpr(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto dataModel = parent->dataModel();

    if (d->srcexpr != QScxmlExecutableContent::NoEvaluator) {
        *ok = false;
        auto v = dataModel->evaluateToString(d->srcexpr, ok);
        if (!*ok)
            return QString();
        return v;
    }

    return QString();
}

QString QScxmlInvokableServiceFactory::calculateId(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto stateMachine = parent->tableData();

    if (d->id != QScxmlExecutableContent::NoString) {
        return stateMachine->string(d->id);
    }

    const QString id
            = QScxmlStateMachinePrivate::generateSessionId(stateMachine->string(d->idPrefix));

    if (d->idlocation != QScxmlExecutableContent::NoString) {
        auto idloc = stateMachine->string(d->idlocation);
        auto ctxt = stateMachine->string(d->invokeLocation);
        *ok = parent->dataModel()->setScxmlProperty(idloc, id, ctxt);
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

    for (const QScxmlExecutableContent::ParameterInfo &param : qAsConst(d->params)) {
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

            auto v = dataModel->scxmlProperty(loc);
            result.insert(name, v);
        }
    }

    for (QScxmlExecutableContent::StringId locid : qAsConst(d->namelist)) {
        QString loc;
        if (locid != QScxmlExecutableContent::NoString) {
            loc = tableData->string(locid);
        }
        if (loc.isEmpty()) {
            // TODO: error message?
            *ok = false;
            return QVariantMap();
        }
        if (dataModel->hasScxmlProperty(loc)) {
            auto v = dataModel->scxmlProperty(loc);
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

class QScxmlScxmlServicePrivate : public QScxmlInvokableServicePrivate
{
public:
    QScxmlScxmlServicePrivate(QScxmlInvokableServiceFactory *factory,
                                QScxmlStateMachine *stateMachine,
                                QScxmlStateMachine *parentStateMachine);
    ~QScxmlScxmlServicePrivate();

    QScxmlStateMachine *stateMachine;
};

QScxmlScxmlServicePrivate::QScxmlScxmlServicePrivate(
        QScxmlInvokableServiceFactory *factory, QScxmlStateMachine *stateMachine,
        QScxmlStateMachine *parentStateMachine) :
    QScxmlInvokableServicePrivate(factory, parentStateMachine), stateMachine(stateMachine)
{}

QScxmlScxmlServicePrivate::~QScxmlScxmlServicePrivate()
{
    delete stateMachine;
}

QScxmlScxmlService::QScxmlScxmlService(QScxmlInvokableServiceFactory *factory,
                                       QScxmlStateMachine *stateMachine,
                                       QScxmlStateMachine *parentStateMachine,
                                       QObject *parent)
    : QScxmlInvokableService(*(new QScxmlScxmlServicePrivate(factory, stateMachine,
                                                             parentStateMachine)), parent)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_parentStateMachine = parentStateMachine;
}

bool QScxmlScxmlService::start()
{
    Q_D(QScxmlScxmlService);
    qCDebug(qscxmlLog) << parentStateMachine() << "preparing to start" << d->stateMachine;

    bool ok = false;
    auto id = factory()->calculateId(parentStateMachine(), &ok);
    if (!ok)
        return false;
    auto data = factory()->calculateData(parentStateMachine(), &ok);
    if (!ok)
        return false;

    QScxmlStateMachinePrivate::get(d->stateMachine)->m_sessionId = id;
    d->stateMachine->setInitialValues(data);
    if (d->stateMachine->init()) {
        qCDebug(qscxmlLog) << parentStateMachine() << "starting" << d->stateMachine;
        d->stateMachine->start();
        return true;
    }

    qCDebug(qscxmlLog) << parentStateMachine() << "failed to start" << d->stateMachine;
    return false;
}

QString QScxmlScxmlService::id() const
{
    Q_D(const QScxmlScxmlService);
    return d->stateMachine->sessionId();
}

QString QScxmlScxmlService::name() const
{
    Q_D(const QScxmlScxmlService);
    return d->stateMachine->name();
}

void QScxmlScxmlService::postEvent(QScxmlEvent *event)
{
    Q_D(QScxmlScxmlService);
    QScxmlStateMachinePrivate::get(d->stateMachine)->postEvent(event);
}

QScxmlStateMachine *QScxmlScxmlService::stateMachine() const
{
    Q_D(const QScxmlScxmlService);
    return d->stateMachine;
}

QScxmlScxmlServiceFactory::QScxmlScxmlServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,
        QScxmlExecutableContent::EvaluatorId srcexpr,
        QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix,
        QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist,
        bool doAutoforward,
        const QVector<QScxmlExecutableContent::ParameterInfo> &params,
        QScxmlExecutableContent::ContainerId finalize)
    : QScxmlInvokableServiceFactory(invokeLocation, srcexpr, id, idPrefix, idlocation, namelist,
                                    doAutoforward, params, finalize)
{}

QScxmlScxmlService *QScxmlScxmlServiceFactory::invokeStatic(QScxmlStateMachine *childStateMachine,
                                                            QScxmlStateMachine *parentStateMachine)
{
    QScxmlStateMachinePrivate::get(childStateMachine)->setIsInvoked(true);
    return new QScxmlScxmlService(this, childStateMachine, parentStateMachine);
}

QScxmlInvokableService *QScxmlDynamicScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    bool ok = true;
    auto srcexpr = calculateSrcexpr(parentStateMachine, &ok);
    if (!ok)
        return Q_NULLPTR;

    return invokeDynamic(parentStateMachine, srcexpr);
}

QT_END_NAMESPACE
