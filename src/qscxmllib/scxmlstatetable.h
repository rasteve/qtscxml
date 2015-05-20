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

typedef int ContainerId;
enum { NoInstruction = -1 };
typedef qint32 StringId;
enum { NoString = -1 };
typedef qint32 ByteArrayId;
typedef qint32 *Instructions;

#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(push, 4) // 4 == sizeof(qint32)
#endif

template <typename T>
struct Array
{
    qint32 count;
    // T[] data;
    T *data() { return const_cast<T *>(const_data()); }
    const T *const_data() const { return reinterpret_cast<const T *>(reinterpret_cast<const char *>(this) + sizeof(Array<T>)); }

    const T &at(int pos) { return *(data() + pos); }
    int dataSize() const { return count * sizeof(T) / sizeof(qint32); }
    int size() const { return sizeof(Array<T>) / sizeof(qint32) + dataSize(); }
};

struct SCXML_EXPORT ByteArray: Array<char>
{
//    static int calculateSize(const QByteArray &str) {
//        return (sizeof(ByteArray) + str.length() * sizeof(char) + 7) & ~0x7;
//    }
};

struct SCXML_EXPORT Param
{
    StringId name;
    DataModel::EvaluatorId expr;
    StringId location;

    static int calculateSize() { return sizeof(Param) / sizeof(qint32); }
};

struct SCXML_EXPORT Instruction
{
    enum InstructionType: qint32 {
        Sequence = 1,
        Sequences,
        Send,
        Raise,
        Log,
        JavaScript,
        Assign,
        If,
        Foreach,
        Cancel,
        Invoke,
        DoneData = 42 // see below
    } instructionType;
};

struct SCXML_EXPORT DoneData: Instruction // ok, this is not an instruction, but it gets allocated there.
{
    StringId contents;
    DataModel::EvaluatorId expr;
    Array<Param> params;

    static InstructionType kind() { return Instruction::DoneData; }
};

struct SCXML_EXPORT InstructionSequence: Instruction
{
    qint32 entryCount; // the amount of qint32's that the instructions take up
    // Instruction[] instructions;

    static InstructionType kind() { return Instruction::Sequence; }
    Instructions instructions() { return reinterpret_cast<Instructions>(this) + sizeof(InstructionSequence) / sizeof(qint32); }
    int size() const { return sizeof(InstructionSequence) / sizeof(qint32) + entryCount; }
};

struct SCXML_EXPORT InstructionSequences: Instruction
{
    qint32 sequenceCount;
    qint32 entryCount; // the amount of qint32's that the sequences take up
    // InstructionSequence[] sequences;

    static InstructionType kind() { return Instruction::Sequences; }
    InstructionSequence *sequences() {
        return reinterpret_cast<InstructionSequence *>(reinterpret_cast<Instructions>(this) + sizeof(InstructionSequences) / sizeof(qint32));
    }
    int size() const { return sizeof(InstructionSequences)/sizeof(qint32) + entryCount; }
    Instructions at(int pos) {
        Instructions seq = reinterpret_cast<Instructions>(sequences());
        while (pos--) {
            seq += reinterpret_cast<InstructionSequence *>(seq)->size();
        }
        return seq;
    }
};

struct SCXML_EXPORT Send: Instruction
{
    StringId instructionLocation;
    ByteArrayId event;
    DataModel::EvaluatorId eventexpr;
    StringId type;
    DataModel::EvaluatorId typeexpr;
    StringId target;
    DataModel::EvaluatorId targetexpr;
    ByteArrayId id;
    StringId idLocation;
    StringId delay;
    DataModel::EvaluatorId delayexpr;
    StringId content;
    Array<StringId> namelist;
//    Array<Param> params;

    static InstructionType kind() { return Instruction::Send; }
    int size() { return sizeof(Send) / sizeof(qint32) + namelist.dataSize() + params()->size(); }
    Array<Param> *params() {
        return reinterpret_cast<Array<Param> *>(
                    reinterpret_cast<Instructions>(this) + sizeof(Send) / sizeof(qint32) + namelist.dataSize());
    }
    static int calculateExtraSize(int paramCount, int nameCount) {
        return 1 + paramCount * sizeof(Param) / sizeof(qint32) + nameCount * sizeof(StringId) / sizeof(qint32);
    }
};

struct SCXML_EXPORT Raise: Instruction
{
    ByteArrayId event;

    static InstructionType kind() { return Instruction::Raise; }
    int size() const { return sizeof(Raise) / sizeof(qint32); }
};

struct SCXML_EXPORT Log: Instruction
{
    StringId label;
    DataModel::EvaluatorId expr;

    static InstructionType kind() { return Instruction::Log; }
    int size() const { return sizeof(Log) / sizeof(qint32); }
};

struct SCXML_EXPORT JavaScript: Instruction
{
    DataModel::EvaluatorId go;

    static InstructionType kind() { return Instruction::JavaScript; }
    int size() const { return sizeof(JavaScript) / sizeof(qint32); }
};

struct SCXML_EXPORT Assign: Instruction
{
    DataModel::EvaluatorId expression;

    static InstructionType kind() { return Instruction::Assign; }
    int size() const { return sizeof(Assign) / sizeof(qint32); }
};

struct SCXML_EXPORT If: Instruction
{
    Array<DataModel::EvaluatorId> conditions;
    // InstructionSequences blocks;
    InstructionSequences *blocks() {
        return reinterpret_cast<InstructionSequences *>(
                    reinterpret_cast<Instructions>(this) + sizeof(If) / sizeof(qint32) + conditions.dataSize());
    }

    static InstructionType kind() { return Instruction::If; }
    int size() { return sizeof(If) / sizeof(qint32) + blocks()->size() + conditions.dataSize(); }
};

struct SCXML_EXPORT Foreach: Instruction
{
    DataModel::EvaluatorId doIt;
    InstructionSequence block;

    static InstructionType kind() { return Instruction::Foreach; }
    int size() const { return sizeof(Foreach) / sizeof(qint32) + block.entryCount; }
    Instructions blockstart() { return reinterpret_cast<Instructions>(&block); }
};

struct SCXML_EXPORT Cancel: Instruction
{
    ByteArrayId sendid;
    DataModel::EvaluatorId sendidexpr;

    static InstructionType kind() { return Instruction::Cancel; }
    int size() const { return sizeof(Cancel) / sizeof(qint32); }
};

struct SCXML_EXPORT Invoke: Instruction
{
    StringId type;
    StringId typeexpr;
    StringId src;
    StringId srcexpr;
    StringId id;
    StringId idLocation;
    Array<StringId> namelist;
    qint32 autoforward;
    Array<Param> params;
    InstructionSequence finalize;

    static InstructionType kind() { return Instruction::Invoke; }
};

#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(pop)
#endif

class SCXML_EXPORT ExecutionEngine
{
public:
    ExecutionEngine(StateTable *table);
    ~ExecutionEngine();

    void setStringTable(const QVector<QString> &strings);
    QString string(StringId id) const;

    void setByteArrayTable(const QVector<QByteArray> &byteArrays);
    QByteArray byteArray(ByteArrayId id) const;

    void setInstructions(Instructions instructions);
    bool execute(ContainerId ip);

    const ExecutableContent::DoneData *doneData(ContainerId id) const;

private:
    bool step(Instructions &ip);

private:
    StateTable *table;
    QVector<QString> strings;
    QVector<QByteArray> byteArrays;
    Instructions instructions;
};

} // ExecutableContent namespace

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
