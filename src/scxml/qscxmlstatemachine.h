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

#ifndef SCXMLSTATEMACHINE_H
#define SCXMLSTATEMACHINE_H

#include <QtScxml/qscxmldatamodel.h>
#include <QtScxml/qscxmlexecutablecontent.h>
#include <QtScxml/qscxmlerror.h>
#include <QtScxml/qscxmlevent.h>

#include <QString>
#include <QVector>
#include <QLoggingCategory>
#include <QUrl>
#include <QVariantList>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
class QTextStream;

Q_SCXML_EXPORT Q_DECLARE_LOGGING_CATEGORY(scxmlLog)

class QScxmlEventBuilder;
class QScxmlInvokableServiceFactory;
class QScxmlInvokableService;
class QScxmlParser;
class QScxmlStateMachine;
class QScxmlTableData;

class Q_SCXML_EXPORT QScxmlEventFilter
{
public:
    virtual ~QScxmlEventFilter();
    virtual bool handle(QScxmlEvent *event, QScxmlStateMachine *stateMachine) = 0;
};

class QScxmlStateMachinePrivate;
class Q_SCXML_EXPORT QScxmlStateMachine: public QObject
{
    Q_DECLARE_PRIVATE(QScxmlStateMachine)
    Q_OBJECT
    Q_ENUMS(BindingMethod)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

protected:
#ifndef Q_QDOC
    QScxmlStateMachine(QObject *parent = 0);
    QScxmlStateMachine(QScxmlStateMachinePrivate &dd, QObject *parent);
#endif // Q_QDOC

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    static QScxmlStateMachine *fromFile(const QString &fileName, QScxmlDataModel *dataModel = Q_NULLPTR);
    static QScxmlStateMachine *fromData(QIODevice *data, const QString &fileName = QString(), QScxmlDataModel *dataModel = Q_NULLPTR);
    QVector<QScxmlError> parseErrors() const;

    QString sessionId() const;
    void setSessionId(const QString &id);
    static QString generateSessionId(const QString &prefix);

    bool isInvoked() const;

    QScxmlDataModel *dataModel() const;
    void setDataModel(QScxmlDataModel *dataModel);

    BindingMethod dataBinding() const;

    bool init(const QVariantMap &initialDataValues = QVariantMap());

    bool isRunning() const;

    QString name() const;
    QStringList activeStates(bool compress = true);
    bool isActive(const QString &scxmlStateName) const;
    bool hasState(const QString &scxmlStateName) const;

    using QObject::connect;
    QMetaObject::Connection connect(const QString &scxmlStateName, const char *signal,
                                    const QObject *receiver, const char *method,
                                    Qt::ConnectionType type = Qt::AutoConnection);

    QScxmlEventFilter *scxmlEventFilter() const;
    void setScxmlEventFilter(QScxmlEventFilter *newFilter);

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);

    Q_INVOKABLE void routeEvent(QScxmlEvent *event);
    Q_INVOKABLE void submitEvent(QScxmlEvent *event);
    Q_INVOKABLE void submitEvent(const QByteArray &eventName);
    Q_INVOKABLE void submitEvent(const QByteArray &event, const QVariant &data);
    void cancelDelayedEvent(const QByteArray &event);

    bool isDispatchableTarget(const QString &target) const;

Q_SIGNALS:
    void runningChanged(bool running);
    void log(const QString &label, const QString &msg);
    void reachedStableState(bool didChange);
    void finished();

public Q_SLOTS:
    void start();

protected: // methods for friends:
    friend QScxmlDataModel;
    friend QScxmlEventBuilder;
    friend QScxmlInvokableServiceFactory;
    friend QScxmlExecutableContent::QScxmlExecutionEngine;

#ifndef Q_QDOC
    void setDataBinding(BindingMethod bindingMethod);
    virtual void setService(const QString &id, QScxmlInvokableService *service);

    QScxmlTableData *tableData() const;
    void setTableData(QScxmlTableData *tableData);
#endif // Q_QDOC
};

QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_H
