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

QT_END_NAMESPACE

namespace Scxml {
SCXML_EXPORT Q_DECLARE_LOGGING_CATEGORY(scxmlLog)
struct XmlNode;

typedef std::function<bool(const QString &)> ErrorDumper;

class ScxmlParser;

bool loopOnSubStates(QState *startState,
                  std::function<bool(QState *)> enteringState = Q_NULLPTR,
                  std::function<void(QState *)> exitingState = Q_NULLPTR,
                  std::function<void(QAbstractState *)> inAbstractState = Q_NULLPTR);

namespace ExecutableContent {
class ExecutionEngine;
}

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

    StateTable(QObject *parent = 0);
    StateTable(StateTablePrivate &dd, QObject *parent);
    StateTablePrivate *privateData();

    int sessionId() const;

    DataModel *dataModel() const;
    void setDataModel(DataModel *dataModel);

    void setDataBinding(BindingMethod b);
    BindingMethod dataBinding() const;

    ExecutableContent::ExecutionEngine *executionEngine() const;

    void doLog(const QString &label, const QString &msg);
    ErrorDumper errorDumper();
    virtual bool init();
    QJSEngine *engine() const;
    void setEngine(QJSEngine *engine);

    QString name() const;
    QStringList currentStates(bool compress = true);

    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid);

    Q_INVOKABLE void submitEvent1(const QString &event) {
        submitEvent(event.toUtf8(), QVariantList());
    }

    Q_INVOKABLE void submitEvent2(const QString &event,  QVariant data) {
        QVariantList dataValues;
        if (data.isValid())
            dataValues << data;
        submitEvent(event.toUtf8(), dataValues);
    }
    void submitEvent(ScxmlEvent *e);
    void submitEvent(const QByteArray &event, const QVariantList &dataValues = QVariantList(),
                     const QStringList &dataNames = QStringList(),
                     ScxmlEvent::EventType type = ScxmlEvent::External,
                     const QByteArray &sendid = QByteArray(), const QString &origin = QString(),
                     const QString &origintype = QString(), const QByteArray &invokeid = QByteArray());
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
    void setDataItemNames(const ExecutableContent::StringIds &dataItemNames);

private:
    Q_DECLARE_PRIVATE(StateTable)
};

// this transition is used only to ensure that the signal is forwarded to the state machine
// (mess with private api instead?), it never triggers, because it will have the incorrect
// precedence as we add it late...
class SCXML_EXPORT ConcreteSignalTransition : public QSignalTransition {
public:
    ConcreteSignalTransition(const QObject *sender, const char *signal,
                             QState *sourceState = 0) :
        QSignalTransition(sender, signal, sourceState) { }
    bool subEventTest(QEvent *event) {
        return QSignalTransition::eventTest(event);
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
    ScxmlInitialState(QState *parent): ScxmlState(parent) { }
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

// Simple basic Xml "dom" to support the scxml xpath data model without qtxml dependency
// add dep?
struct SCXML_EXPORT XmlNode {
    bool isText() const;
    QStringList texts() const;
    QString text() const;
    void addText(const QString &value);

    void addTag(const QString &name, const QString &xmlns, const QXmlStreamAttributes &attributes,
                QVector<XmlNode> childs);

    XmlNode() { }
    XmlNode(const QString &value) : m_name(QStringLiteral("$") + value) { }
    XmlNode(const QString &name, const QString &xmlns, const QXmlStreamAttributes &attributes,
            const QVector<XmlNode> &childs)
        : m_name(name), m_namespace(xmlns), m_attributes(attributes), m_childs(childs) { }
    QString name() const;
    QXmlStreamAttributes attributes() const;
    bool loopOnAttributes(std::function<bool(const QXmlStreamAttribute &)> l);
    bool loopOnText(std::function<bool(const QString &)> l) const;
    bool loopOnChilds(std::function<bool(const XmlNode &)> l) const;
    bool loopOnTags(std::function<bool(const XmlNode &)> l) const;
    void dump(QXmlStreamWriter &s) const;
protected:
    QString m_name;
    QString m_namespace;
    QXmlStreamAttributes m_attributes;
    QVector<XmlNode> m_childs;
};

} // namespace Scxml

#endif // SCXMLSTATETABLE_H
