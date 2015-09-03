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

#include <QtScxml/datamodel.h>
#include <QtScxml/executablecontent.h>
#include <QtScxml/scxmlevent.h>

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

namespace Scxml {
Q_SCXML_EXPORT Q_DECLARE_LOGGING_CATEGORY(scxmlLog)

class ScxmlParser;

namespace ExecutableContent {
class ExecutionEngine;
}

class Q_SCXML_EXPORT TableData
{
public:
    virtual ~TableData();

    virtual QString string(ExecutableContent::StringId id) const = 0;
    virtual QByteArray byteArray(ExecutableContent::ByteArrayId id) const = 0;
    virtual ExecutableContent::Instructions instructions() const = 0;
    virtual EvaluatorInfo evaluatorInfo(EvaluatorId evaluatorId) const = 0;
    virtual AssignmentInfo assignmentInfo(EvaluatorId assignmentId) const = 0;
    virtual ForeachInfo foreachInfo(EvaluatorId foreachId) const = 0;
    virtual ExecutableContent::StringId *dataNames(int *count) const = 0;

    virtual ExecutableContent::ContainerId initialSetup() const = 0;
    virtual QString name() const = 0;
};

class Q_SCXML_EXPORT ScxmlError
{
public:
    ScxmlError();
    ScxmlError(const QString &fileName, int line, int column, const QString &description);
    ScxmlError(const ScxmlError &);
    ScxmlError &operator=(const ScxmlError &);
    ~ScxmlError();

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

QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const ScxmlError &error);
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<ScxmlError> &errors);

class Q_SCXML_EXPORT ScxmlInvokableService
{
public:
    ScxmlInvokableService(const QString &id, const QVariantMap &data, bool autoforward,
                          ExecutableContent::ContainerId finalize, StateMachine *parent);
    virtual ~ScxmlInvokableService();

    QString id() const;
    bool autoforward() const;
    QVariantMap data() const;
    StateMachine *parent() const;

    virtual void submitEvent(QScxmlEvent *event) = 0;

    void finalize();

private:
    class Data;
    Data *d;
};

class Q_SCXML_EXPORT ScxmlInvokableServiceFactory
{
public:
    struct Q_SCXML_EXPORT Param
    {
        ExecutableContent::StringId name;
        EvaluatorId expr;
        ExecutableContent::StringId location;
    };

    ScxmlInvokableServiceFactory(ExecutableContent::StringId invokeLocation,
                                 ExecutableContent::StringId id,
                                 ExecutableContent::StringId idPrefix,
                                 ExecutableContent::StringId idlocation,
                                 const QVector<ExecutableContent::StringId> &namelist,
                                 bool autoforward,
                                 const QVector<Param> &params,
                                 ExecutableContent::ContainerId finalizeContent);
    virtual ~ScxmlInvokableServiceFactory();

    virtual ScxmlInvokableService *invoke(StateMachine *parent) = 0;

protected:
    QString calculateId(StateMachine *parent, bool *ok) const;
    QVariantMap calculateData(StateMachine *parent, bool *ok) const;
    bool autoforward() const;
    ExecutableContent::ContainerId finalizeContent() const;

private:
    class Data;
    Data *d;
};

class Q_SCXML_EXPORT InvokableScxmlServiceFactory: public ScxmlInvokableServiceFactory
{
public:
    InvokableScxmlServiceFactory(Scxml::ExecutableContent::StringId invokeLocation,
                                 Scxml::ExecutableContent::StringId id,
                                 Scxml::ExecutableContent::StringId idPrefix,
                                 Scxml::ExecutableContent::StringId idlocation,
                                 const QVector<Scxml::ExecutableContent::StringId> &namelist,
                                 bool autoforward,
                                 const QVector<Param> &params,
                                 ExecutableContent::ContainerId finalize)
        : ScxmlInvokableServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist,
                                       autoforward, params, finalize)
    {}

protected:
    ScxmlInvokableService *finishInvoke(StateMachine *child, StateMachine *parent);
};

class StateMachine;
class Q_SCXML_EXPORT ScxmlEventFilter
{
public:
    virtual ~ScxmlEventFilter();
    virtual bool handle(QScxmlEvent *event, StateMachine *stateMachine) = 0;
};

class StateMachinePrivate;
class Q_SCXML_EXPORT StateMachine: public QObject
{
    Q_OBJECT
    Q_ENUMS(BindingMethod)

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    static StateMachine *fromFile(const QString &fileName, DataModel *dataModel = Q_NULLPTR);
    static StateMachine *fromData(QIODevice *data, const QString &fileName = QString(), DataModel *dataModel = Q_NULLPTR);
    QVector<ScxmlError> errors() const;

    StateMachine(QObject *parent = 0);
    StateMachine(StateMachinePrivate &dd, QObject *parent);

    /// If this state machine is backed by a QStateMachine, that QStateMachine is returned. Otherwise, a null-pointer is returned.
    QStateMachine *qStateMachine() const;

    QString sessionId() const;
    void setSessionId(const QString &id);
    static QString generateSessionId(const QString &prefix);

    bool isInvoked() const;
    void setIsInvoked(bool invoked);

    DataModel *dataModel() const;
    void setDataModel(DataModel *dataModel);

    void setDataBinding(BindingMethod b);
    BindingMethod dataBinding() const;

    TableData *tableData() const;
    void setTableData(TableData *tableData);

    void doLog(const QString &label, const QString &msg);

    StateMachine *parentStateMachine() const;
    void setParentStateMachine(StateMachine *parent);

    Q_INVOKABLE bool init();

    bool isRunning() const;

    QString name() const;
    QStringList activeStates(bool compress = true);
    bool isActive(const QString &scxmlStateName) const;
    bool hasState(const QString &scxmlStateName) const;

    using QObject::connect;
    QMetaObject::Connection connect(const QString &scxmlStateName, const char *signal,
                                    const QObject *receiver, const char *method,
                                    Qt::ConnectionType type = Qt::AutoConnection);

    ScxmlEventFilter *scxmlEventFilter() const;
    void setScxmlEventFilter(ScxmlEventFilter *newFilter);

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);

    Q_INVOKABLE void routeEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(QScxmlEvent *e);
    Q_INVOKABLE void submitEvent(const QByteArray &event);
    Q_INVOKABLE void submitEvent(const QByteArray &event, const QVariant &data);
    void cancelDelayedEvent(const QByteArray &event);

    bool isLegalTarget(const QString &target) const;
    bool isDispatchableTarget(const QString &target) const;

    void registerService(ScxmlInvokableService *service);
    void unregisterService(ScxmlInvokableService *service);

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

private:
    Q_DECLARE_PRIVATE(StateMachine)
};

} // namespace Scxml

Q_DECLARE_TYPEINFO(Scxml::ScxmlError, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_H
