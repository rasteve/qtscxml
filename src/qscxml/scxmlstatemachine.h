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

#include <QScxml/datamodel.h>
#include <QScxml/executablecontent.h>
#include <QScxml/scxmlevent.h>

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
SCXML_EXPORT Q_DECLARE_LOGGING_CATEGORY(scxmlLog)

class ScxmlParser;

namespace ExecutableContent {
class ExecutionEngine;
}

class SCXML_EXPORT TableData
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

class Q_QML_EXPORT ScxmlError
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

QDebug Q_QML_EXPORT operator<<(QDebug debug, const ScxmlError &error);

class StateMachinePrivate;
class SCXML_EXPORT StateMachine: public QObject
{
    Q_OBJECT
    Q_ENUMS(BindingMethod)

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    static StateMachine *fromFile(const QString &fileName, DataModel *dataModel = Q_NULLPTR);
    static StateMachine *fromData(QIODevice *data, DataModel *dataModel = Q_NULLPTR);
    QVector<ScxmlError> errors() const;

    StateMachine(QObject *parent = 0);
    StateMachine(StateMachinePrivate &dd, QObject *parent);

    /// If this state machine is backed by a QStateMachine, that QStateMachine is returned. Otherwise, a null-pointer is returned.
    QStateMachine *qStateMachine() const;

    int sessionId() const;

    DataModel *dataModel() const;
    void setDataModel(DataModel *dataModel);

    void setDataBinding(BindingMethod b);
    BindingMethod dataBinding() const;

    ExecutableContent::ExecutionEngine *executionEngine() const;
    TableData *tableData() const;
    void setTableData(TableData *tableData);

    void doLog(const QString &label, const QString &msg);

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

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);
    Q_INVOKABLE void submitEvent1(const QString &event);
    Q_INVOKABLE void submitEvent2(const QString &event,  QVariant data);

    void submitEvent(ScxmlEvent *e);
    Q_INVOKABLE void submitEvent(const QByteArray &event,
                                 const QVariantList &dataValues = QVariantList(),
                                 const QStringList &dataNames = QStringList(),
                                 ScxmlEvent::EventType type = ScxmlEvent::External,
                                 const QByteArray &sendid = QByteArray(),
                                 const QString &origin = QString(),
                                 const QString &origintype = QString(),
                                 const QByteArray &invokeid = QByteArray());
    void submitDelayedEvent(int delayInMiliSecs,
                            ScxmlEvent *e);
    void cancelDelayedEvent(const QByteArray &event);

    bool isLegalTarget(const QString &target) const;
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
    void executeInitialSetup();

private:
    Q_DECLARE_PRIVATE(StateMachine)
};

} // namespace Scxml

Q_DECLARE_TYPEINFO(Scxml::ScxmlError, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // SCXMLSTATEMACHINE_H
