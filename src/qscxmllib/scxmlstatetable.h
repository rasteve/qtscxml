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
class StateTable;

class DataModelPrivate;
class SCXML_EXPORT DataModel
{
    Q_DISABLE_COPY(DataModel)

public:
    struct Data {
        Data();
        Data(const QString &id, const QString &src, const QString &expr, QState *context);

        QString id;
        QString src;
        QString expr;
        QState *context = nullptr;
    };

    typedef qint32 EvaluatorId;
    enum { NoEvaluator = -1 };

public:
    DataModel(StateTable *table);
    virtual ~DataModel();

    StateTable *table() const;
    QVector<Data> data() const;
    void addData(const Data &data);

    virtual void setup() = 0;
    virtual void initializeDataFor(QState *state) = 0;

    virtual EvaluatorId createToStringEvaluator(const QString &expr, const QString &context) = 0;
    virtual EvaluatorId createToBoolEvaluator(const QString &expr, const QString &context) = 0;
    virtual EvaluatorId createToVariantEvaluator(const QString &expr, const QString &context) = 0;
    virtual EvaluatorId createScriptEvaluator(const QString &script, const QString &context) = 0;
    virtual EvaluatorId createAssignmentEvaluator(const QString &dest, const QString &expr,
                                                  const QString &context) = 0;
    virtual EvaluatorId createForeachEvaluator(const QString &array, const QString &item,
                                               const QString &index, const QString &context) = 0;

    virtual QString evaluateToString(EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateToBool(EvaluatorId id, bool *ok) = 0;
    virtual QVariant evaluateToVariant(EvaluatorId id, bool *ok) = 0;
    virtual void evaluateToVoid(EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateForeach(EvaluatorId id, bool *ok, std::function<bool()> body) = 0;

    virtual void setEvent(const ScxmlEvent &event) = 0;

    virtual QVariant property(const QString &name) const = 0;
    virtual bool hasProperty(const QString &name) const = 0;
    virtual void setStringProperty(const QString &name, const QString &value, const QString &context,
                                   bool *ok) = 0;

private:
    DataModelPrivate *d;
};

namespace ExecutableContent {
class ExecutionEngine;
}

class StateTablePrivate;
class SCXML_EXPORT StateTable: public QStateMachine
{
    Q_OBJECT
    Q_ENUMS(BindingMethod)

    static QAtomicInt m_sessionIdCounter;

public:
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    StateTable(QObject *parent = 0);
    StateTable(StateTablePrivate &dd, QObject *parent);
    ~StateTable();

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

public:
    // use q_property for these?
    ScxmlEvent _event;
    QString _name;
    typedef QHash<QString, QString> Dict;
    QStringList currentStates(bool compress = true);
    void setInitialSetup(ExecutableContent::ContainerId sequence);

private:
    Q_DECLARE_PRIVATE(StateTable)
    DataModel *m_dataModel;
    const int m_sessionId;
    ExecutableContent::ContainerId m_initialSetup = ExecutableContent::NoInstruction;
    QJSEngine *m_engine;
    BindingMethod m_dataBinding;
    ExecutableContent::ExecutionEngine *m_executionEngine;
    bool m_warnIndirectIdClashes;
    friend class StateTableBuilder;

    struct QueuedEvent { QEvent *event; EventPriority priority; };
    QVector<QueuedEvent> *m_queuedEvents;
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

/*class EventTransition : public QKeyEventTransition { };
Qt::NoModifier      0x00000000 No modifier key is pressed.
Qt::ShiftModifier   0x02000000 A Shift key on the keyboard is pressed.
Qt::ControlModifier 0x04000000 A Ctrl key on the keyboard is pressed.
Qt::AltModifier     0x08000000 An Alt key on the keyboard is pressed.
Qt::MetaModifier    0x10000000 A Meta key on the keyboard is pressed.
Qt::KeypadModifier  0x20000000 A keypad button is pressed.
Qt::GroupSwitchModifier

QMouseEventTransition // no, add?
*/

class SCXML_EXPORT ScxmlBaseTransition : public QAbstractTransition {
    Q_OBJECT
public:
    typedef QSharedPointer<ConcreteSignalTransition> TransitionPtr;
    ScxmlBaseTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>());
    ScxmlBaseTransition(QAbstractTransitionPrivate &dd, QState *parent,
                        const QList<QByteArray> &eventSelector = QList<QByteArray>());
    StateTable *table() const;

    QString transitionLocation() const;

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    virtual bool clear();
    virtual bool init();

    QList<QByteArray> eventSelector;
protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;
private:
    QList<TransitionPtr> m_concreteTransitions;
};

class SCXML_EXPORT ScxmlTransition : public ScxmlBaseTransition {
    Q_OBJECT
public:
    typedef QSharedPointer<ConcreteSignalTransition> TransitionPtr;
    ScxmlTransition(QState * sourceState = 0, const QList<QByteArray> &eventSelector = QList<QByteArray>(),
                    DataModel::EvaluatorId conditionalExp = DataModel::NoEvaluator);

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    StateTable *table() const;

    DataModel::EvaluatorId conditionalExp = DataModel::NoEvaluator;
    ScxmlEvent::EventType type;
    ExecutableContent::ContainerId instructionsOnTransition = ExecutableContent::NoInstruction;

protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;
};

class SCXML_EXPORT ScxmlState: public QState
{
    Q_OBJECT
public:
    ScxmlState(QState *parent = 0)
        : QState(parent)
        , m_dataInitialized(false)
    {}
    ScxmlState(QStatePrivate &dd, QState *parent = 0)
        : QState(dd, parent)
        , m_dataInitialized(false)
    {}
    StateTable *table() const;
    virtual bool init();
    QString stateLocation() const;

    ExecutableContent::ContainerId onEntryInstructions = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onExitInstructions = ExecutableContent::NoInstruction;

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;
    bool m_dataInitialized;
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
public:
    ScxmlFinalState(QState *parent = 0);
    StateTable *table() const;
    virtual bool init();

    ExecutableContent::ContainerId doneData() const;
    void setDoneData(ExecutableContent::ContainerId doneData);

    ExecutableContent::StringId location = ExecutableContent::NoString;
    ExecutableContent::ContainerId onEntryInstructions = ExecutableContent::NoInstruction;
    ExecutableContent::ContainerId onExitInstructions = ExecutableContent::NoInstruction;

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    ExecutableContent::ContainerId m_doneData = ExecutableContent::NoInstruction;
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
