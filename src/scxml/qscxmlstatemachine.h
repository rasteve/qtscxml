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
#include <QtScxml/qscxmlevent.h>

#include <QState>
#include <QFinalState>
#include <QAbstractState>
#include <QAbstractTransition>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QXmlStreamAttributes>
#include <QRegExp>
#include <QStateMachine>
#include <QJSEngine>
#include <QPointer>
#include <QSharedPointer>
#include <QMetaMethod>
#include <QJsonValue>
#include <QSignalTransition>
#include <QLoggingCategory>
#include <QAbstractTransition>
#include <QUrl>
#include <QVariantList>
#include <functional>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
class QTextStream;
class QDebug;

Q_SCXML_EXPORT Q_DECLARE_LOGGING_CATEGORY(scxmlLog)

class Q_SCXML_EXPORT QScxmlTableData
{
public:
    virtual ~QScxmlTableData();

    virtual QString string(QScxmlExecutableContent::StringId id) const = 0;
    virtual QByteArray byteArray(QScxmlExecutableContent::ByteArrayId id) const = 0;
    virtual QScxmlExecutableContent::Instructions instructions() const = 0;
    virtual QScxmlExecutableContent::EvaluatorInfo evaluatorInfo(QScxmlExecutableContent::EvaluatorId evaluatorId) const = 0;
    virtual QScxmlExecutableContent::AssignmentInfo assignmentInfo(QScxmlExecutableContent::EvaluatorId assignmentId) const = 0;
    virtual QScxmlExecutableContent::ForeachInfo foreachInfo(QScxmlExecutableContent::EvaluatorId foreachId) const = 0;
    virtual QScxmlExecutableContent::StringId *dataNames(int *count) const = 0;

    virtual QScxmlExecutableContent::ContainerId initialSetup() const = 0;
    virtual QString name() const = 0;
};

class Q_SCXML_EXPORT QScxmlError
{
public:
    QScxmlError();
    QScxmlError(const QString &fileName, int line, int column, const QString &description);
    QScxmlError(const QScxmlError &);
    QScxmlError &operator=(const QScxmlError &);
    ~QScxmlError();

    bool isValid() const;

    QString fileName() const;
    int line() const;
    int column() const;
    QString description() const;

    QString toString() const;

private:
    class ScxmlErrorPrivate;
    ScxmlErrorPrivate *d;
};

QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QScxmlError &error);
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<QScxmlError> &errors);

class Q_SCXML_EXPORT QScxmlInvokableService
{
public:
    QScxmlInvokableService(const QString &id, const QVariantMap &data, bool autoforward,
                           QScxmlExecutableContent::ContainerId finalize, QScxmlStateMachine *parent);
    virtual ~QScxmlInvokableService();

    QString id() const;
    bool autoforward() const;
    QVariantMap data() const;
    QScxmlStateMachine *parent() const;

    virtual void submitEvent(QScxmlEvent *event) = 0;

    void finalize();

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

protected:
    QString calculateId(QScxmlStateMachine *parent, bool *ok) const;
    QVariantMap calculateData(QScxmlStateMachine *parent, bool *ok) const;
    bool autoforward() const;
    QScxmlExecutableContent::ContainerId finalizeContent() const;

private:
    class Data;
    Data *d;
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

    QScxmlStateMachine(QObject *parent = 0);
    QScxmlStateMachine(QScxmlStateMachinePrivate &dd, QObject *parent);

    QString sessionId() const;
    void setSessionId(const QString &id);
    static QString generateSessionId(const QString &prefix);

    bool isInvoked() const;
    void setIsInvoked(bool invoked);

    QScxmlDataModel *dataModel() const;
    void setDataModel(QScxmlDataModel *dataModel);

    void setDataBinding(BindingMethod b);
    BindingMethod dataBinding() const;

    QScxmlTableData *tableData() const;
    void setTableData(QScxmlTableData *tableData);

    void doLog(const QString &label, const QString &msg);

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

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);

    Q_INVOKABLE void routeEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(const QByteArray &event);
    Q_INVOKABLE void submitEvent(const QByteArray &event, const QVariant &data);
    void cancelDelayedEvent(const QByteArray &event);

    bool isLegalTarget(const QString &target) const;
    bool isDispatchableTarget(const QString &target) const;

    void registerService(QScxmlInvokableService *service);
    void unregisterService(QScxmlInvokableService *service);

Q_SIGNALS:
    void log(const QString &label, const QString &msg);
    void reachedStableState(bool didChange);
    void finished();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onFinished();

protected:
    void executeInitialSetup();
};

Q_DECLARE_TYPEINFO(QScxmlError, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_H
