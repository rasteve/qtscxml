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

#ifndef QSCXMLINVOKABLESERVICE_H
#define QSCXMLINVOKABLESERVICE_H

#include <QtScxml/qscxmldatamodel.h>

#include <QString>
#include <QVariantMap>

QT_BEGIN_NAMESPACE

class QScxmlEvent;
class QScxmlStateMachine;
class QScxmlInvokableServiceFactory;

class Q_SCXML_EXPORT QScxmlInvokableService
{
public:
    QScxmlInvokableService(QScxmlInvokableServiceFactory *service, QScxmlStateMachine *parent);
    virtual ~QScxmlInvokableService();

    bool autoforward() const;
    QScxmlStateMachine *parent() const;

    virtual bool start() = 0;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual void postEvent(QScxmlEvent *event) = 0;

    void finalize();

protected:
    QScxmlInvokableServiceFactory *service() const;

private:
    class Data;
    Data *d;
};

class Q_SCXML_EXPORT QScxmlInvokableServiceFactory
{
public:
    struct Q_SCXML_EXPORT Param
    {
        QScxmlExecutableContent::StringId name;
        QScxmlExecutableContent::EvaluatorId expr;
        QScxmlExecutableContent::StringId location;

        Param(QScxmlExecutableContent::StringId theName,
              QScxmlExecutableContent::EvaluatorId theExpr,
              QScxmlExecutableContent::StringId theLocation)
            : name(theName)
            , expr(theExpr)
            , location(theLocation)
        {}

        Param()
            : name(QScxmlExecutableContent::NoString)
            , expr(QScxmlExecutableContent::NoInstruction)
            , location(QScxmlExecutableContent::NoString)
        {}
    };

    QScxmlInvokableServiceFactory(QScxmlExecutableContent::StringId invokeLocation,
                                  QScxmlExecutableContent::StringId id,
                                  QScxmlExecutableContent::StringId idPrefix,
                                  QScxmlExecutableContent::StringId idlocation,
                                  const QVector<QScxmlExecutableContent::StringId> &namelist,
                                  bool autoforward,
                                  const QVector<Param> &params,
                                  QScxmlExecutableContent::ContainerId finalizeContent);
    virtual ~QScxmlInvokableServiceFactory();

    virtual QScxmlInvokableService *invoke(QScxmlStateMachine *parent) = 0;

public: // callbacks from the service:
    QString calculateId(QScxmlStateMachine *parent, bool *ok) const;
    QVariantMap calculateData(QScxmlStateMachine *parent, bool *ok) const;
    bool autoforward() const;
    QScxmlExecutableContent::ContainerId finalizeContent() const;

private:
    class Data;
    Data *d;
};

class Q_SCXML_EXPORT QScxmlInvokableScxml: public QScxmlInvokableService
{
public:
    QScxmlInvokableScxml(QScxmlInvokableServiceFactory *service,
                         QScxmlStateMachine *stateMachine,
                         QScxmlStateMachine *parent);
    virtual ~QScxmlInvokableScxml();

    bool start() Q_DECL_OVERRIDE;
    QString id() const Q_DECL_OVERRIDE;
    QString name() const Q_DECL_OVERRIDE;
    void postEvent(QScxmlEvent *event) Q_DECL_OVERRIDE;

    QScxmlStateMachine *stateMachine() const;

private:
    QScxmlStateMachine *m_stateMachine;
};

class Q_SCXML_EXPORT QScxmlInvokableScxmlServiceFactory: public QScxmlInvokableServiceFactory
{
public:
    QScxmlInvokableScxmlServiceFactory(QScxmlExecutableContent::StringId invokeLocation,
                                       QScxmlExecutableContent::StringId id,
                                       QScxmlExecutableContent::StringId idPrefix,
                                       QScxmlExecutableContent::StringId idlocation,
                                       const QVector<QScxmlExecutableContent::StringId> &namelist,
                                       bool doAutoforward,
                                       const QVector<Param> &params,
                                       QScxmlExecutableContent::ContainerId finalize);

protected:
    QScxmlInvokableService *finishInvoke(QScxmlStateMachine *child, QScxmlStateMachine *parent);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QScxmlInvokableService *)

#endif // QSCXMLINVOKABLESERVICE_H
