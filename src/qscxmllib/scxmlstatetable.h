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
#include "scxmlglobals.h"
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

class SCXML_EXPORT ScxmlEvent: public QEvent {
public:
    static QEvent::Type scxmlEventType;
    enum EventType { Platform, Internal, External };

    ScxmlEvent(const QByteArray &name = QByteArray(), EventType eventType = External,
               const QVariantList &dataValues = QVariantList(), const QStringList &dataNames = QStringList(),
               const QByteArray &sendid = QByteArray (),
               const QString &origin = QString (), const QString &origintype = QString (),
               const QByteArray &invokeid = QByteArray());

    QByteArray name() const { return m_name; }
    EventType eventType() const { return m_type; }
    QString scxmlType() const;
    QByteArray sendid() const { return m_sendid; }
    QString origin() const { return m_origin; }
    QString origintype() const { return m_origintype; }
    QByteArray invokeid() const { return m_invokeid; }
    QVariantList dataValues() const { return m_dataValues; }
    QStringList dataNames() const { return m_dataNames; }
    void reset(const QByteArray &name, EventType eventType = External,
               QVariantList dataValues = QVariantList(), const QByteArray &sendid = QByteArray(),
               const QString &origin = QString(), const QString &origintype = QString(),
               const QByteArray &invokeid = QByteArray());
    void clear();

private:
    QByteArray m_name;
    EventType m_type;
    QVariantList m_dataValues; // extra data
    QStringList m_dataNames; // extra data
    QByteArray m_sendid; // if set, or id of <send> if failure
    QString m_origin; // uri to answer by setting the target of send, empty for internal and platform events
    QString m_origintype; // type to answer by setting the type of send, empty for internal and platform events
    QByteArray m_invokeid; // id of the invocation that triggered the child process if this was invoked
};


typedef std::function<bool(const QString &)> ErrorDumper;

class ScxmlParser;

bool loopOnSubStates(QState *startState,
                  std::function<bool(QState *)> enteringState = Q_NULLPTR,
                  std::function<void(QState *)> exitingState = Q_NULLPTR,
                  std::function<void(QAbstractState *)> inAbstractState = Q_NULLPTR);
class StateTable;

namespace ExecutableContent {

// dirty mix: AST, but directly executable...
struct SCXML_EXPORT Instruction {
public:
    typedef QSharedPointer<Instruction> Ptr; // avoid smart pointer and simply delete in InstructionSequence destructor?

    virtual ~Instruction() { }

    virtual bool execute(StateTable *table) const = 0;
    virtual bool init(StateTable *table);
    virtual void clear() { }
    virtual bool bind() { return true; }
};

struct SCXML_EXPORT InstructionSequence : public Instruction {
    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

    bool bind() Q_DECL_OVERRIDE;

    QList<Instruction::Ptr> statements;
};

struct SCXML_EXPORT InstructionSequences
{
    typedef QList<InstructionSequence *> InstructionSequenceList;
    typedef InstructionSequenceList::const_iterator const_iterator;

    ~InstructionSequences();

    InstructionSequence *newInstructions();
    bool init(StateTable *table);
    void execute(StateTable *table);
    const_iterator begin() const;
    const_iterator end() const;
    int size() const;
    bool isEmpty() const;
    const InstructionSequence *at(int idx) const;
    InstructionSequence *last() const;
    void append(InstructionSequence *s);

    InstructionSequenceList sequences;
};

} // namespace ExecutableContent

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

    typedef std::function<QString (bool *)> ToStringEvaluator;
    typedef std::function<bool (bool *)> ToBoolEvaluator;
    typedef std::function<QVariant (bool *)> ToVariantEvaluator;
    typedef std::function<void (bool *)> ToVoidEvaluator;
    typedef std::function<bool (bool *, std::function<bool ()>)> ForeachEvaluator;

public:
    DataModel(StateTable *table);
    virtual ~DataModel();

    StateTable *table() const;
    QVector<Data> data() const;
    void addData(const Data &data);

    virtual void setup() = 0;
    virtual void initializeDataFor(QState *state) = 0;
    virtual ToStringEvaluator createToStringEvaluator(const QString &expr, const QString &context) = 0;
    virtual ToBoolEvaluator createToBoolEvaluator(const QString &expr, const QString &context) = 0;
    virtual ToVariantEvaluator createToVariantEvaluator(const QString &expr, const QString &context) = 0;
    virtual ToVoidEvaluator createScriptEvaluator(const QString &script, const QString &context) = 0;
    virtual ToVoidEvaluator createAssignmentEvaluator(const QString &dest, const QString &expr,
                                                      const QString &context) = 0;
    virtual ForeachEvaluator createForeachEvaluator(const QString &array, const QString &item,
                                                    const QString &index, const QString &context) = 0;

    virtual void setEvent(const ScxmlEvent &event) = 0;

    virtual QVariant property(const QString &name) const = 0;
    virtual bool hasProperty(const QString &name) const = 0;
    virtual void setStringProperty(const QString &name, const QString &value) = 0;

private:
    DataModelPrivate *d;
};

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

    void addId(const QByteArray&,QObject*); // FIXME: remove

    DataModel *dataModel() const;
    void setDataModel(DataModel *dataModel);

    void setDataBinding(BindingMethod b) {
        m_dataBinding = b;
    }
    BindingMethod dataBinding() const {
        return m_dataBinding;
    }

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
    QString _sessionid;
    QString _name;
    typedef QHash<QString, QString> Dict;
    QStringList _ioprocessors;
    QStringList currentStates(bool compress = true);
    void setInitialSetup(const ExecutableContent::InstructionSequence &sequence);

private:
    Q_DECLARE_PRIVATE(StateTable)
    DataModel *m_dataModel;
    const int m_sessionId;
    ExecutableContent::InstructionSequence m_initialSetup;
    QJSEngine *m_engine;
    BindingMethod m_dataBinding;
    bool m_warnIndirectIdClashes;
    friend class StateTableBuilder;

    struct QueuedEvent { QEvent *event; EventPriority priority; };
    QVector<QueuedEvent> *m_queuedEvents;
};

namespace ExecutableContent {

class SCXML_EXPORT Param
{
public:
    Param();
    Param(const QString &name, const DataModel::ToVariantEvaluator &expr, const QString &location);

    bool evaluate(StateTable *table, QVariantList &dataValues, QStringList &dataNames) const;
    static bool evaluate(const QVector<Param> &params, StateTable *table, QVariantList &dataValues, QStringList &dataNames);

private:
    QString name;
    DataModel::ToVariantEvaluator expr = nullptr;
    QString location;
};

class SCXML_EXPORT DoneData
{
public:
    DoneData();
    DoneData(const QString &contents, const DataModel::ToStringEvaluator &expr, const QVector<Param> &params);

    QString contents() const;
    DataModel::ToStringEvaluator expr() const;
    QVector<Param> params() const;

private:
    QString m_contents;
    DataModel::ToStringEvaluator m_expr = nullptr;
    QVector<Param> m_params;
};

struct SCXML_EXPORT Send : public Instruction {
    QString instructionLocation;
    QByteArray event;
    DataModel::ToStringEvaluator eventexpr = nullptr;
    QString type;
    DataModel::ToStringEvaluator typeexpr = nullptr;
    QString target;
    DataModel::ToStringEvaluator targetexpr = nullptr;
    QString id;
    QString idLocation;
    QString delay;
    DataModel::ToStringEvaluator delayexpr = nullptr;
    QStringList namelist;
    QVector<Param> params;
    QString content;
    bool execute(StateTable *table) const Q_DECL_OVERRIDE;
};

class SCXML_EXPORT Raise : public Instruction
{
public:
    Raise(const QByteArray &event);

    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    QByteArray event;
};

class SCXML_EXPORT Log : public Instruction
{
public:
    Log(const QString &label, const DataModel::ToStringEvaluator &expr);

    virtual bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    QString label;
    DataModel::ToStringEvaluator expr;
};

struct SCXML_EXPORT Script : public Instruction {
};

class SCXML_EXPORT JavaScript : public Script
{
public:
    JavaScript(const DataModel::ToVoidEvaluator &go);

    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    DataModel::ToVoidEvaluator go;
};

class SCXML_EXPORT AssignExpression : public Instruction
{
public:
    AssignExpression(const DataModel::ToVoidEvaluator &expression);

    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    QString location;
    DataModel::ToVoidEvaluator expression;
};

struct SCXML_EXPORT If : public Instruction {
    QVector<DataModel::ToBoolEvaluator> conditions;
    InstructionSequences blocks;
    bool execute(StateTable *table) const Q_DECL_OVERRIDE;
};

class SCXML_EXPORT Foreach : public Instruction
{
public:
    Foreach(const DataModel::ForeachEvaluator &it, const InstructionSequence &block);

    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    DataModel::ForeachEvaluator doIt = nullptr;
    InstructionSequence block;
};

class SCXML_EXPORT Cancel : public Instruction
{
public:
    Cancel(const QByteArray &sendid, const DataModel::ToStringEvaluator &sendidexpr);
    bool execute(StateTable *table) const Q_DECL_OVERRIDE;

private:
    QByteArray sendid;
    DataModel::ToStringEvaluator sendidexpr;
};

struct SCXML_EXPORT Invoke : public Instruction {
    QString type;
    QString typeexpr;
    QString src;
    QString srcexpr;
    QString id;
    QString idLocation;
    QStringList namelist;
    bool autoforward;
    QList<Param> params;
    XmlNode *content;
    InstructionSequence finalize;
    bool execute(StateTable *table) const Q_DECL_OVERRIDE;
};

} // namespace ExecutableContent
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
                    const DataModel::ToBoolEvaluator &conditionalExp = nullptr);

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    StateTable *table() const;

    DataModel::ToBoolEvaluator conditionalExp;
    ScxmlEvent::EventType type;
    ExecutableContent::InstructionSequence instructionsOnTransition;
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

    ExecutableContent::InstructionSequences onEntryInstructions;
    ExecutableContent::InstructionSequences onExitInstructions;
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

    const ExecutableContent::DoneData &doneData() const;
    void setDoneData(const ExecutableContent::DoneData &doneData);

    ExecutableContent::InstructionSequences onEntryInstructions;
    ExecutableContent::InstructionSequences onExitInstructions;

protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;

private:
    ExecutableContent::DoneData m_doneData;
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
