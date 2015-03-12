/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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
               const QVariantList &datas = QVariantList(), const QStringList &dataNames = QStringList(),
               const QByteArray &sendid = QByteArray (),
               const QString &origin = QString (), const QString &origintype = QString (),
               const QByteArray &invokeid = QByteArray());

    QByteArray name() const { return m_name; }
    QString scxmlType() const;
    QByteArray sendid() const { return m_sendid; }
    QString origin() const { return m_origin; }
    QString origintype() const { return m_origintype; }
    QByteArray invokeid() const { return m_invokeid; }
    QVariant data() const;
    QVariantList datas() const { return m_datas; }
    void reset(const QByteArray &name, EventType eventType = External,
               QVariantList datas = QVariantList(), const QByteArray &sendid = QByteArray(),
               const QString &origin = QString(), const QString &origintype = QString(),
               const QByteArray &invokeid = QByteArray());
    void clear();
    QJSValue jsValue(QJSEngine *engine) const;

private:
    QByteArray m_name;
    EventType m_type;
    QVariantList m_datas; // extra data
    QStringList m_dataNames; // extra data
    QByteArray m_sendid; // if set, or id of <send> if failure
    QString m_origin; // uri to answer by setting the target of send, empty for internal and platform events
    QString m_origintype; // type to answer by setting the type of send, empty for internal and platform events
    QByteArray m_invokeid; // id of the invocation that triggered the child process if this was invoked
};


typedef std::function<bool(const QString &)> ErrorDumper;
struct ScxmlData {
    QString id;
    QString src;
    QString expr;
    QState *context;
};

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
    enum Kind {
        Raise,
        Send,
        Log,
        JavaScript,
        AssignJson,
        AssignExpression,
        If,
        Foreach,
        Cancel,
        Invoke,
        Sequence
    };
    Instruction(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : parentState(parentState), transition(transition) {
        if (!parentState && !transition)
            qCDebug(scxmlLog) << "scxml: unbound instruction";
    }

    virtual ~Instruction() { }

    StateTable *table() const;
    QString instructionLocation();

    virtual void execute() = 0;
    virtual Kind instructionKind() const = 0;
    virtual bool init();
    virtual void clear() { }
    virtual bool bind() { return true; }

    QAbstractState *parentState;
    QAbstractTransition *transition;
};

struct SCXML_EXPORT InstructionSequence : public Instruction {
    InstructionSequence(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    void execute() Q_DECL_OVERRIDE;

    Kind instructionKind() const Q_DECL_OVERRIDE {
        return Sequence;
    }
    bool bind() Q_DECL_OVERRIDE;

    QList<Instruction::Ptr> statements;
};

} // namespace ExecutableContent

class StateTablePrivate;
class SCXML_EXPORT StateTable: public QStateMachine
{
    Q_OBJECT
    Q_ENUMS(DataModelType BindingMethod)
public:
    enum DataModelType {
        Javascript,
        Json,
        Xml,
        None
    };
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    StateTable(QObject *parent = 0);
    StateTable(StateTablePrivate &dd, QObject *parent);

    StateTable *table() {
        return this;
    }

    // returns the value corresponding to the given scxml id
    template <typename T>
    T *idToValue(const QByteArray &idVal, bool strict = false) {
        QObject *obj = 0;
        if (m_idObjects.contains(idVal)) {
            obj = m_idObjects.value(idVal).data();
            if (!obj)
                m_idObjects.remove(idVal); // expensive to remove the other side in m_objectIds without storing more
        }
        if (strict || obj)
            return qobject_cast<T *>(obj); // if we match a key do not look further even if object is different
        qCDebug(scxmlLog) << "non cached scxml id access attempt for value " << idVal;
        if (idVal.contains('.')) {
            QList<QByteArray> pieces = idVal.split('.');
            QObject *newVal = idToValue<QObject>(pieces.at(0));
            pieces.removeFirst();
            while (newVal && !pieces.isEmpty()) {
                if (StateTable *newSt = qobject_cast<StateTable *>(newVal)) {
                    T *res = newSt->idToValue<T>(pieces.join('.'));
                    if (res) {
                        m_idObjects[idVal] = res; // do not cache?
                        m_objectIds[res] = idVal;
                    }
                    return res;
                } else if (newVal) {
                    QByteArray pathPiece = pieces.first();
                    pieces.removeFirst();
                    if (pieces.isEmpty()) {
                        T *res = newVal->findChild<T *>(QString::fromUtf8(pathPiece));
                        if (res) {
                            m_idObjects[idVal] = res; // do not cache?
                            m_objectIds[res] = idVal;
                        }
                        return res;
                    }
                    newVal = newVal->findChild<QObject *>(QString::fromUtf8(pathPiece));
                }
            }
            Q_ASSERT(!newVal);
            return 0;
        }
        T *res = this->findChild<T *>(QString::fromUtf8(idVal));
        if (res) {
            m_idObjects[idVal] = res; // do not cache?
            m_objectIds[res] = idVal;
        }
        return res;
    }

    bool addId(const QByteArray &idStr, QObject *value,
               std::function<bool(const QString &)> errorDumper = Q_NULLPTR, bool overwrite = false);

    QJSValue datamodelJSValues() const;

    // tries to build an scxml id for the given object
    QByteArray objectId(QObject *obj, bool strict = false);

    void setDataModel(DataModelType dt) {
        m_dataModel = dt;
    }
    DataModelType dataModel() const {
        return m_dataModel;
    }
    void setDataBinding(BindingMethod b) {
        m_dataBinding = b;
    }
    BindingMethod dataBinding() const {
        return m_dataBinding;
    }

    void initializeDataFor(QState *);

    //void toScxml(QXmlStreamWriter &dumper);
    //void toCpp(QTextStream &dumper);
    //void toQml(QTextStream &qml);
    //bool writeDiffs(std::function<bool(const QString &)> diffDumper, const StateTable &other);
    void doLog(const QString &label, const QString &msg);
    QString evalValueStr(const QString &expr, std::function<QString()> context,
                         const QString &defaultValue = QString());
    int evalValueInt(const QString &expr, std::function<QString()> context,
                     int defaultValue);
    bool evalValueBool(const QString &expr, std::function<QString()> context,
                       bool defaultValue = false);
    QJSValue evalJSValue(const QString &expr, std::function<QString()> context,
                         QJSValue defaultValue = QJSValue(), bool noRaise = false);
    ErrorDumper errorDumper();
    virtual bool init();
    QJSEngine *engine() const;
    void setEngine(QJSEngine *engine);
    Q_INVOKABLE void submitError(const QByteArray &type, const QString &msg) {
        qCDebug(scxmlLog) << "machine " << _name << " had error " << type << ":" << msg;
        submitEvent(type, QVariantList() << QVariant(msg));
    }

    Q_INVOKABLE void submitEvent1(const QString &event) {
        submitEvent(event.toUtf8(), QVariantList());
    }

    Q_INVOKABLE void submitEvent2(const QString &event,  QVariant data) {
        QVariantList datas;
        if (data.isValid())
            datas << data;
        submitEvent(event.toUtf8(), datas);
    }
    void submitEvent(const QByteArray &event, const QVariantList &datas = QVariantList(),
                     const QStringList &dataNames = QStringList(),
                     ScxmlEvent::EventType type = ScxmlEvent::External,
                     const QByteArray &sendid = QByteArray(), const QString &origin = QString(),
                     const QString &origintype = QString(), const QByteArray &invokeid = QByteArray());
    void submitDelayedEvent(int delay, const QByteArray &event, const QVariantList &datas = QVariantList(),
                            const QStringList &dataNames = QStringList(),
                            ScxmlEvent::EventType type = ScxmlEvent::External,
                            const QByteArray &sendid = QByteArray(), const QString &origin = QString(),
                            const QString &origintype = QString(), const QByteArray &invokeid = QByteArray());

signals:
    void log(const QString &label, const QString &msg);
    void reachedStableState(bool didChange);
protected:
    void beginSelectTransitions(QEvent *event) Q_DECL_OVERRIDE;
    void beginMicrostep(QEvent *event) Q_DECL_OVERRIDE;
    void endMicrostep(QEvent *event) Q_DECL_OVERRIDE;
    virtual void assignEvent();
    void setupDataModel();

public:
    // use q_property for these?
    ScxmlEvent _event;
    QString _sessionid;
    QString _name;
    typedef QHash<QString, QString> Dict;
    QStringList _ioprocessors;
    QList<QByteArray> currentStates(bool compress = true);
private:
    Q_DECLARE_PRIVATE(StateTable)
    ExecutableContent::InstructionSequence m_initialSetup;
    QHash<QByteArray, QPointer<QObject> > m_idObjects;
    QHash<QObject *, QByteArray> m_objectIds;
    DataModelType m_dataModel;
    QVector<ScxmlData> m_data;
    QJSEngine *m_engine;
    QJSValue m_dataModelJSValues;
    BindingMethod m_dataBinding;
    bool m_warnIndirectIdClashes;
    friend class ScxmlParser;
};

namespace ExecutableContent {

struct SCXML_EXPORT Param {
    QString name;
    QString expr;
    QString location;
};

struct SCXML_EXPORT DoneData {
    XmlNode *content;
    QVector<Param> params;
};

struct SCXML_EXPORT Send : public Instruction {
    Send(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QByteArray event;
    QString eventexpr;
    QString type;
    QString typeexpr;
    QString target;
    QString targetexpr;
    QString id;
    QString idLocation;
    QString delay;
    QString delayexpr;
    QStringList namelist;
    QVector<Param> params;
    XmlNode *content;
    ~Send();
    void execute() Q_DECL_OVERRIDE { Q_ASSERT(false); /*to implement*/ }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Send; }
};

struct SCXML_EXPORT Raise : public Instruction {
    Raise(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QByteArray event;
    void execute() Q_DECL_OVERRIDE {
        table()->submitEvent(event, QVariantList(), QStringList(), ScxmlEvent::Internal); }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Raise; }
};

struct SCXML_EXPORT Log : public Instruction {
    Log(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QString label;
    QString expr;
    virtual void execute() Q_DECL_OVERRIDE {
        table()->doLog(label, table()->evalValueStr(expr,
                                                    [this]()->QString { return instructionLocation(); },
                                                    QString()));
    }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Log; }
};

struct SCXML_EXPORT Script : public Instruction {
    Script(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QString source;
    QString src;
};

struct SCXML_EXPORT JavaScript : public Script {
    JavaScript(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Script(parentState, transition) { }
    QJSValue compiledFunction;
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::JavaScript; }
    void execute() Q_DECL_OVERRIDE;
};

struct SCXML_EXPORT AssignExpression : public Instruction {
    AssignExpression(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QString location;
    QString expression;
    XmlNode *content;
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::AssignExpression; }
    void execute() Q_DECL_OVERRIDE {
        if (table() && table()->engine())
            table()->datamodelJSValues().setProperty(
                        location, table()->evalJSValue(expression,
                                                       [this]() -> QString {
                                                           return QStringLiteral("%1 with expression %2")
                                                           .arg(instructionLocation(), expression);
                                                       }));
    }
};

struct SCXML_EXPORT AssignJson : public Instruction {
    AssignJson(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QString location;
    QJsonValue value;
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::AssignJson; }
};

struct SCXML_EXPORT If : public Instruction {
    If(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QStringList conditions;
    QVector<InstructionSequence> blocks;
    void execute() Q_DECL_OVERRIDE;
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::If; }
};

struct SCXML_EXPORT Foreach : public Instruction {
    Foreach(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition), block(parentState, transition) { }
    QString array;
    QString item;
    QString index;
    InstructionSequence block;
    void execute() Q_DECL_OVERRIDE { Q_ASSERT(false); }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Foreach; }
};

struct SCXML_EXPORT Cancel : public Instruction {
    Cancel(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
    QString sendid;
    QString sendidexpr;
    void execute() Q_DECL_OVERRIDE { Q_ASSERT(false); }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Cancel; }
};

struct SCXML_EXPORT Invoke : public Instruction {
    Invoke(QAbstractState *parentState = 0, QAbstractTransition *transition = 0)
        : Instruction(parentState, transition) { }
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
    void execute() Q_DECL_OVERRIDE { Q_ASSERT(false); }
    Kind instructionKind() const Q_DECL_OVERRIDE { return Instruction::Invoke; }
};

class SCXML_EXPORT InstructionVisitor {
protected:
    virtual void visitRaise(Raise *) = 0;
    virtual void visitLog(Log*) = 0;
    virtual void visitSend(Send *) = 0;
    virtual void visitJavaScript(JavaScript *) = 0;
    virtual void visitAssignJson(AssignJson *) = 0;
    virtual void visitAssignExpression(AssignExpression *) = 0;
    virtual void visitCancel(Cancel *) = 0;

    virtual bool visitInvoke(Invoke *) = 0;
    virtual void endVisitInvoke(Invoke *) = 0;
    virtual bool visitIf(If *) = 0;
    virtual void endVisitIf(If *) = 0;
    virtual bool visitForeach(Foreach *) = 0;
    virtual void endVisitForeach(Foreach *) = 0;
    virtual bool visitSequence(InstructionSequence *) = 0;
    virtual void endVisitSequence(InstructionSequence *) = 0;
public:
    void accept(Instruction *instruction);
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

    virtual QList<QByteArray> targetIds() const;

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
                    const QList<QByteArray> &targetIds = QList<QByteArray>(),
                    const QString &conditionalExp = QString());

    bool eventTest(QEvent *event) Q_DECL_OVERRIDE;
    bool init() Q_DECL_OVERRIDE;
    QList<QByteArray> targetIds() const  Q_DECL_OVERRIDE { return m_targetIds; }

    QString conditionalExp;
    ExecutableContent::InstructionSequence instructionsOnTransition;
protected:
    void onTransition(QEvent *event) Q_DECL_OVERRIDE;
private:
    QList<QByteArray> m_targetIds;
};

class SCXML_EXPORT ScxmlState: public QState
{
    Q_OBJECT
public:
    ScxmlState(QState *parent = 0)
        : QState(parent), onEntryInstruction(this) , onExitInstruction(this)
        , m_dataInitialized(false) { }
    ScxmlState(QStatePrivate &dd, QState *parent = 0)
        : QState(dd, parent), onEntryInstruction(this), onExitInstruction(this)
        , m_dataInitialized(false) { }
    StateTable *table() const;
    virtual bool init();
    QString stateLocation() const;

    ExecutableContent::InstructionSequence onEntryInstruction;
    ExecutableContent::InstructionSequence onExitInstruction;
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
    virtual bool init() Q_DECL_OVERRIDE;
};


class SCXML_EXPORT ScxmlFinalState: public QFinalState
{
    Q_OBJECT
public:
    ScxmlFinalState(QState *parent) : QFinalState(parent) { }
    StateTable *table() const;
    virtual bool init();

    ExecutableContent::InstructionSequence onEntryInstruction;
    ExecutableContent::InstructionSequence onExitInstruction;
    ExecutableContent::DoneData doneData;
protected:
    void onEntry(QEvent * event) Q_DECL_OVERRIDE;
    void onExit(QEvent * event) Q_DECL_OVERRIDE;
    QJSValue jsDoneData();
private:
    QJSValue m_jsDoneData;
    friend class ScxmlParser;
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
