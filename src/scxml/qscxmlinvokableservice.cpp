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

/*!
 * \class QScxmlInvokableService
 * \brief Service invoked from \l{SCXML Specification}{SCXML}
 * \since 5.8
 * \inmodule QtScxml
 *
 * QScxmlInvokableService is the base class for services called from state
 * machines via the mechanism described in
 * \l {SCXML Specification - 6.4 <invoke>}. This represents an actual instance
 * of an invoked service.
 */

/*!
 * \class QScxmlInvokableServiceFactory
 * \brief Factory for creating instances of QScxmlInvokableService
 * \since 5.8
 * \inmodule QtScxml
 *
 * A factory for creating instances of QScxmlInvokableService. This reperesents
 * an \c <invoke> element in the SCXML document. Each time the service is
 * actually invoked a new instance of QScxmlInvokableService is created.
 */

/*!
  \property QScxmlInvokableServiceFactory::invokeInfo

  The QScxmlExecutableContent::InvokeInfo passed to the constructor.
 */

/*!
  \property QScxmlInvokableServiceFactory::names

  The QVector<QScxmlExecutableContent::StringId> of names passed to the
  constructor.
 */

/*!
  \property QScxmlInvokableServiceFactory::parameters

  The QVector<QScxmlExecutableContent::ParameterInfo> passed to the
  constructor.
 */

/*!
 * \class QScxmlStaticScxmlServiceFactory
 * \brief Factory for creating QScxmlScxmlService instances from precompiled
 *        documents
 * \since 5.8
 * \inmodule QtScxml
 *
 * A factory for instantiating QScxmlStateMachines from files known at compile
 * time, that is files specified via the \c src attribute in \c <invoke>
 */

/*!
 * \class QScxmlDynamicScxmlServiceFactory
 * \brief Factory for QScxmlScxmlService instances created from documents
 *        loaded at run time
 * \since 5.8
 * \inmodule QtScxml
 *
 * A QScxmlScxmlServiceFactory for creating dynamically resolved services. This
 * is used when loading \l{SCXML Specification}{SCXML} content from files a
 * parent state machine requests at runtime, via the \c srcexpr attribute in
 * \c <invoke>.
 */

/*!
 * \property QScxmlInvokableService::parentStateMachine
 *
 * The QScxmlStateMachine that invoked the service.
 */

/*!
 * \property QScxmlInvokableService::id
 *
 * The id specified in the \c id attribute of the \c <invoke> element.
 */

/*!
 * \property QScxmlInvokableService::name
 *
 * The name of the service being invoked.
 */

/*!
 * \fn QScxmlInvokableService::postEvent(QScxmlEvent *event)
 *
 * Sends an \a event to the service.
 */

/*!
 * \fn QScxmlInvokableService::start()
 *
 * Starts the invokable service. Returns \c true on success, or \c false if the
 * invocation fails.
 */

/*!
 * \fn QScxmlInvokableServiceFactory::invoke(QScxmlStateMachine *parentStateMachine)
 *
 * Invoke the service with the parameters given in the contructor, passing
 * \a parentStateMachine as parent. Returns the new QScxmlInvokableService.
 */

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

QScxmlStaticScxmlServiceFactoryPrivate::QScxmlStaticScxmlServiceFactoryPrivate(
        const QMetaObject *metaObject,
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters)
    : QScxmlInvokableServiceFactoryPrivate(invokeInfo, names, parameters), metaObject(metaObject)
{
}

QScxmlInvokableService::QScxmlInvokableService(QScxmlStateMachine *parentStateMachine,
                                               QScxmlInvokableServiceFactory *factory) :
    QObject(*(new QScxmlInvokableServicePrivate(parentStateMachine)), factory)
{
}

QScxmlStateMachine *QScxmlInvokableService::parentStateMachine() const
{
    Q_D(const QScxmlInvokableService);
    return d->parentStateMachine;
}

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QObject(*(new QScxmlInvokableServiceFactoryPrivate(invokeInfo, names, parameters)), parent)
{}

const QScxmlExecutableContent::InvokeInfo &QScxmlInvokableServiceFactory::invokeInfo() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->invokeInfo;
}

const QVector<QScxmlExecutableContent::ParameterInfo> &
QScxmlInvokableServiceFactory::parameters() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->parameters;
}

const QVector<QScxmlExecutableContent::StringId> &QScxmlInvokableServiceFactory::names() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->names;
}

QString calculateSrcexpr(QScxmlStateMachine *parent, QScxmlExecutableContent::EvaluatorId srcexpr,
                         bool *ok)
{
    Q_ASSERT(ok);
    *ok = true;
    auto dataModel = parent->dataModel();

    if (srcexpr != QScxmlExecutableContent::NoEvaluator) {
        *ok = false;
        auto v = dataModel->evaluateToString(srcexpr, ok);
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

QScxmlScxmlService::~QScxmlScxmlService()
{
    delete stateMachine;
}

/*!
  Create a QScxmlScxmlService wrapping \a stateMachine, invoked from
  \a parentStateMachine, as child of \a parent.
 */
QScxmlScxmlService::QScxmlScxmlService(QScxmlStateMachine *stateMachine,
                                       QScxmlStateMachine *parentStateMachine,
                                       QScxmlInvokableServiceFactory *factory)
    : QScxmlInvokableService(parentStateMachine, factory), stateMachine(stateMachine)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_parentStateMachine = parentStateMachine;
}

/*!
 * \reimp
 */
bool QScxmlScxmlService::start()
{
    Q_D(QScxmlInvokableService);
    qCDebug(qscxmlLog) << parentStateMachine() << "preparing to start" << stateMachine;

    const QScxmlInvokableServiceFactory *factory
            = qobject_cast<QScxmlInvokableServiceFactory *>(parent());
    Q_ASSERT(factory);

    bool ok = false;
    auto id = d->calculateId(parentStateMachine(), factory->invokeInfo(), &ok);
    if (!ok)
        return false;
    auto data = d->calculateData(parentStateMachine(), factory->parameters(), factory->names(),
                                 &ok);
    if (!ok)
        return false;

    QScxmlStateMachinePrivate::get(stateMachine)->m_sessionId = id;
    stateMachine->setInitialValues(data);
    if (stateMachine->init()) {
        qCDebug(qscxmlLog) << parentStateMachine() << "starting" << stateMachine;
        stateMachine->start();
        return true;
    }

    qCDebug(qscxmlLog) << parentStateMachine() << "failed to start" << stateMachine;
    return false;
}

/*!
  \reimp
 */
QString QScxmlScxmlService::id() const
{
    return stateMachine->sessionId();
}

/*!
  \reimp
 */
QString QScxmlScxmlService::name() const
{
    return stateMachine->name();
}

/*!
  \reimp
 */
void QScxmlScxmlService::postEvent(QScxmlEvent *event)
{
    QScxmlStateMachinePrivate::get(stateMachine)->postEvent(event);
}

/*!
  Create a QScxmlDynamicScxmlServiceFactory, passing the attributes of the
  \c <invoke> element as \a invokeInfo, any \c <param> child elements as
  \a parameters, the content of the \c names attribute as \a names, and the
  QObject parent \a parent.
 */
QScxmlDynamicScxmlServiceFactory::QScxmlDynamicScxmlServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &names,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QScxmlInvokableServiceFactory(invokeInfo, names, parameters, parent)
{}

/*!
  \reimp
 */
QScxmlInvokableService *QScxmlDynamicScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    bool ok = true;
    auto srcexpr = calculateSrcexpr(parentStateMachine, invokeInfo().expr, &ok);
    if (!ok)
        return Q_NULLPTR;

    return invokeDynamicScxmlService(srcexpr, parentStateMachine, this);
}

QScxmlStaticScxmlServiceFactory::QScxmlStaticScxmlServiceFactory(
        const QMetaObject *metaObject,
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QVector<QScxmlExecutableContent::StringId> &nameList,
        const QVector<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QScxmlInvokableServiceFactory(*(new QScxmlStaticScxmlServiceFactoryPrivate(
                                      metaObject, invokeInfo, nameList, parameters)), parent)
{
}

/*!
  \reimp
 */
QScxmlInvokableService *QScxmlStaticScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    Q_D(const QScxmlStaticScxmlServiceFactory);
    QScxmlStateMachine *instance = qobject_cast<QScxmlStateMachine *>(
                d->metaObject->newInstance(Q_ARG(QObject *, this)));
    return instance ? invokeStaticScxmlService(instance, parentStateMachine, this) : nullptr;
}

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        QScxmlInvokableServiceFactoryPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

QScxmlScxmlService *invokeStaticScxmlService(QScxmlStateMachine *childStateMachine,
                                             QScxmlStateMachine *parentStateMachine,
                                             QScxmlInvokableServiceFactory *factory)
{
    QScxmlStateMachinePrivate::get(childStateMachine)->setIsInvoked(true);
    return new QScxmlScxmlService(childStateMachine, parentStateMachine, factory);
}

QT_END_NAMESPACE
