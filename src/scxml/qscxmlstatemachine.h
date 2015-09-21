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

class QScxmlParser;
class QScxmlTableData;
class QScxmlStateMachine;
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

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    static QScxmlStateMachine *fromFile(const QString &fileName, QScxmlDataModel *dataModel = Q_NULLPTR);
    static QScxmlStateMachine *fromData(QIODevice *data, const QString &fileName = QString(), QScxmlDataModel *dataModel = Q_NULLPTR);
    QVector<QScxmlError> errors() const;

    QString sessionId() const;
    void setSessionId(const QString &id);
    static QString generateSessionId(const QString &prefix);

    bool isInvoked() const;

    QScxmlDataModel *dataModel() const;
    void setDataModel(QScxmlDataModel *dataModel);

    void setDataBinding(BindingMethod b);
    BindingMethod dataBinding() const;

    QScxmlTableData *tableData() const;
    void setTableData(QScxmlTableData *tableData);

    QScxmlStateMachine *parentStateMachine() const;
    void setParentStateMachine(QScxmlStateMachine *parent);

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

    void doLog(const QString &label, const QString &msg);

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);

    Q_INVOKABLE void routeEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(const QByteArray &event);
    Q_INVOKABLE void submitEvent(const QByteArray &event, const QVariant &data);
    void cancelDelayedEvent(const QByteArray &event);

    bool isDispatchableTarget(const QString &target) const;

Q_SIGNALS:
    void log(const QString &label, const QString &msg);
    void reachedStableState(bool didChange);
    void finished();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onFinished();

protected:
    friend class QScxmlParser; // For the constructor:
    QScxmlStateMachine(QObject *parent = 0);
    QScxmlStateMachine(QScxmlStateMachinePrivate &dd, QObject *parent);

    void executeInitialSetup();
};

QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_H
