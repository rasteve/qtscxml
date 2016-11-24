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
#include "qscxmlinvokableservice_p.h"
#include "qscxmlstatemachine_p.h"

QT_BEGIN_NAMESPACE

QScxmlInvokableServicePrivate::QScxmlInvokableServicePrivate(QScxmlStateMachine *parentStateMachine)
    : parentStateMachine(parentStateMachine)
{
    static int metaType = qRegisterMetaType<QScxmlInvokableService *>();
    Q_UNUSED(metaType);
}

QScxmlInvokableServiceFactoryPrivate::QScxmlInvokableServiceFactoryPrivate(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &namelist,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters)
    : invokeInfo(invokeInfo)
    , names(namelist)
    , parameters(parameters)
{}

QScxmlInvokableService::QScxmlInvokableService(QScxmlStateMachine *parentStateMachine,
                                               QObject *parent) :
    QObject(*(new QScxmlInvokableServicePrivate(parentStateMachine)), parent)
{
}

QScxmlStateMachine *QScxmlInvokableService::parentStateMachine() const
{
    Q_D(const QScxmlInvokableService);
    return d->parentStateMachine;
}

void QScxmlInvokableService::finalize(QScxmlExecutableContent::ContainerId finalize)
{
    if (finalize != QScxmlExecutableContent::NoContainer) {
        auto psm = parentStateMachine();
        qCDebug(qscxmlLog) << psm << "running finalize on event";
        auto smp = QScxmlStateMachinePrivate::get(psm);
        smp->m_executionEngine->execute(finalize);
    }
}

QScxmlInvokableService::QScxmlInvokableService(QScxmlInvokableServicePrivate &dd, QObject *parent) :
    QObject(dd, parent)
{
}

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters)
    : d(new QScxmlInvokableServiceFactoryPrivate(invokeInfo, names, parameters))
{}

QScxmlInvokableServiceFactory::~QScxmlInvokableServiceFactory()
{
    delete d;
}

QScxmlExecutableContent::InvokeInfo QScxmlInvokableServiceFactory::invokeInfo() const
{
    return d->invokeInfo;
}

QVector<QScxmlExecutableContent::ParameterInfo> QScxmlInvokableServiceFactory::parameters() const
{
    return d->parameters;
}

QVector<QScxmlExecutableContent::StringId> QScxmlInvokableServiceFactory::names() const
{
    return d->names;
}

QString QScxmlInvokableServiceFactoryPrivate::calculateSrcexpr(QScxmlStateMachine *parent,
                                                               bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto dataModel = parent->dataModel();

    if (invokeInfo.expr != QScxmlExecutableContent::NoEvaluator) {
        *ok = false;
        auto v = dataModel->evaluateToString(invokeInfo.expr, ok);
        if (!*ok)
            return QString();
        return v;
    }

    return QString();
}

QString QScxmlInvokableServicePrivate::calculateId(
        QScxmlStateMachine *parent, const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto stateMachine = parent->tableData();

    if (invokeInfo.id != QScxmlExecutableContent::NoString) {
        return stateMachine->string(invokeInfo.id);
    }

    const QString newId = QScxmlStateMachinePrivate::generateSessionId(
                stateMachine->string(invokeInfo.prefix));

    if (invokeInfo.location != QScxmlExecutableContent::NoString) {
        auto idloc = stateMachine->string(invokeInfo.location);
        auto ctxt = stateMachine->string(invokeInfo.context);
        *ok = parent->dataModel()->setScxmlProperty(idloc, newId, ctxt);
        if (!*ok)
            return QString();
    }

    return newId;
}

QVariantMap QScxmlInvokableServicePrivate::calculateData(
        QScxmlStateMachine *parent,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters,
        const QVector<QScxmlExecutableContent::StringId> &names,
        bool *ok) const
{
    Q_ASSERT(ok);

    QVariantMap result;
    auto dataModel = parent->dataModel();
    auto tableData = parent->tableData();

    for (const QScxmlExecutableContent::ParameterInfo &param : parameters) {
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

    for (QScxmlExecutableContent::StringId locid : names) {
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

QScxmlScxmlServicePrivate::QScxmlScxmlServicePrivate(QScxmlStateMachine *stateMachine,
                                                     QScxmlStateMachine *parentStateMachine) :
    QScxmlInvokableServicePrivate(parentStateMachine), stateMachine(stateMachine)
{}

QScxmlScxmlServicePrivate::~QScxmlScxmlServicePrivate()
{
    delete stateMachine;
}

QScxmlScxmlService::QScxmlScxmlService(QScxmlStateMachine *stateMachine,
                                       QScxmlStateMachine *parentStateMachine,
                                       QObject *parent)
    : QScxmlInvokableService(*(new QScxmlScxmlServicePrivate(stateMachine, parentStateMachine)),
                             parent)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_parentStateMachine = parentStateMachine;
}

bool QScxmlScxmlService::start(const QScxmlExecutableContent::InvokeInfo &invokeInfo,
                               const QVector<QScxmlExecutableContent::ParameterInfo> &parameters,
                               const QVector<QScxmlExecutableContent::StringId> &names)
{
    Q_D(QScxmlScxmlService);
    qCDebug(qscxmlLog) << parentStateMachine() << "preparing to start" << d->stateMachine;

    bool ok = false;
    auto id = d->calculateId(parentStateMachine(), invokeInfo, &ok);
    if (!ok)
        return false;
    auto data = d->calculateData(parentStateMachine(), parameters, names, &ok);
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
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters)
    : QScxmlInvokableServiceFactory(invokeInfo, names, parameters)
{}

QScxmlScxmlService *QScxmlScxmlServiceFactory::invokeStatic(QScxmlStateMachine *childStateMachine,
                                                            QScxmlStateMachine *parentStateMachine)
{
    QScxmlStateMachinePrivate::get(childStateMachine)->setIsInvoked(true);
    return new QScxmlScxmlService(childStateMachine, parentStateMachine);
}

QScxmlDynamicScxmlServiceFactory::QScxmlDynamicScxmlServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters)
    : QScxmlScxmlServiceFactory(invokeInfo, names, parameters)
{}

QScxmlInvokableService *QScxmlDynamicScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    bool ok = true;
    auto srcexpr = d->calculateSrcexpr(parentStateMachine, &ok);
    if (!ok)
        return Q_NULLPTR;

    return invokeDynamic(parentStateMachine, srcexpr);
}

QT_END_NAMESPACE
