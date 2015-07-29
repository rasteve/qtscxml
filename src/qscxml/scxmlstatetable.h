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

#ifndef SCXMLSTATETABLE_H
#define SCXMLSTATETABLE_H

#include "datamodel.h"
#include "executablecontent.h"
#include "scxmlevent.h"

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
QT_END_NAMESPACE

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

class ScxmlState;
class StateTablePrivate;
class SCXML_EXPORT StateTable: public QStateMachine
{
    Q_OBJECT
    Q_ENUMS(BindingMethod)

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    static StateTable *fromFile(const QString &fileName, DataModel *dataModel = Q_NULLPTR);
    static StateTable *fromData(QIODevice *data, DataModel *dataModel = Q_NULLPTR);
    QVector<ScxmlError> errors() const;

    StateTable(QObject *parent = 0);
    StateTable(StateTablePrivate &dd, QObject *parent);

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
    void queueEvent(ScxmlEvent *event, EventPriority priority);
    void submitQueuedEvents();

    bool isLegalTarget(const QString &target) const;
    bool isDispatchableTarget(const QString &target) const;

Q_SIGNALS:
    void log(const QString &label, const QString &msg);
    void reachedStableState(bool didChange);

private Q_SLOTS:
    void onFinished();

protected:
    void beginSelectTransitions(QEvent *event) Q_DECL_OVERRIDE;
    void beginMicrostep(QEvent *event) Q_DECL_OVERRIDE;
    void endMicrostep(QEvent *event) Q_DECL_OVERRIDE;
    void executeInitialSetup();

protected: // friend interface
    friend class StateTableBuilder;
    void setName(const QString &name);
    void setInitialSetup(ExecutableContent::ContainerId sequence);

private:
    Q_DECLARE_PRIVATE(StateTable)
};

// this transition is used only to ensure that the signal is forwarded to the state machine
// (mess with private api instead?), it never triggers, because it will have the incorrect
// precedence as we add it late...
class SCXML_EXPORT ConcreteSignalTransition : public QSignalTransition {
public:
    ConcreteSignalTransition(const QObject *asender, const char *asignal,
                             QState *theSourceState = 0) :
        QSignalTransition(asender, asignal, theSourceState) { }
    bool subEventTest(QEvent *anevent) {
        return QSignalTransition::eventTest(anevent);
    }
protected:
    bool eventTest(QEvent *) Q_DECL_OVERRIDE {
        return false;
    }
};

class SCXML_EXPORT ScxmlBaseTransition : public QAbstractTransition
{
    Q_OBJECT
    class Data;

public:
    typedef QSharedPointer<ConcreteSignalTransition> TransitionPtr;

    ScxmlBaseTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                        const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ~ScxmlBaseTransition();

    StateTable *table() const;
    QString transitionLocation() const;

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    virtual bool clear();
    virtual bool init();

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class SCXML_EXPORT ScxmlTransition : public ScxmlBaseTransition
{
    Q_OBJECT
    class Data;

public:
    ScxmlTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ScxmlTransition(const QList<QByteArray> &eventSelector);
    ~ScxmlTransition();

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    StateTable *table() const;

    void setInstructionsOnTransition(ExecutableContent::ContainerId instructions);
    void setConditionalExpression(EvaluatorId evaluator);

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class SCXML_EXPORT ScxmlState: public QState
{
    Q_OBJECT
    class Data;

public:
    ScxmlState(QState *parent = 0);
    ~ScxmlState();

    StateTable *table() const;
    virtual bool init();
    QString stateLocation() const;

    void setInitInstructions(ExecutableContent::ContainerId instructions);
    void setOnEntryInstructions(ExecutableContent::ContainerId instructions);
    void setOnExitInstructions(ExecutableContent::ContainerId instructions);

Q_SIGNALS:
    void didEnter();
    void willExit();

protected:
    ScxmlState(QStatePrivate &dd, QState *parent = 0);

    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

class SCXML_EXPORT ScxmlInitialState: public ScxmlState
{
    Q_OBJECT
public:
    ScxmlInitialState(QState *theParent): ScxmlState(theParent) { }
};

class SCXML_EXPORT ScxmlFinalState: public QFinalState
{
    Q_OBJECT
    class Data;
public:
    ScxmlFinalState(QState *parent = 0);
    ~ScxmlFinalState();

    StateTable *table() const;
    virtual bool init();

    ExecutableContent::ContainerId doneData() const;
    void setDoneData(ExecutableContent::ContainerId doneData);

    void setOnEntryInstructions(ExecutableContent::ContainerId instructions);
    void setOnExitInstructions(ExecutableContent::ContainerId instructions);

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    Data *d;
};

} // namespace Scxml

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Scxml::ScxmlError, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // SCXMLSTATETABLE_H
