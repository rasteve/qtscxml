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
    virtual void submitEvent(QScxmlEvent *event) = 0;

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
    void submitEvent(QScxmlEvent *event) Q_DECL_OVERRIDE;

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
