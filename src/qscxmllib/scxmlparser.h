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

#ifndef SCXMLPARSER_H
#define SCXMLPARSER_H

#include "scxmlstatetable.h"

#include <QStringList>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamAttributes;
class QXmlStreamReader;
class QHistoryState;
class QFile;
QT_END_NAMESPACE

namespace Scxml {
class ScxmlParser;

namespace DocumentModel {

struct XmlLocation
{
    int line;
    int column;

    XmlLocation(int line, int column): line(line), column(column) {}
};

struct If;
struct Send;
struct Invoke;
struct Script;
struct AbstractState;
struct State;
struct Transition;
struct HistoryState;
struct Scxml;
class NodeVisitor;
struct Node {
    XmlLocation xmlLocation;

    Node(const XmlLocation &xmlLocation): xmlLocation(xmlLocation) {}
    virtual ~Node();
    virtual void accept(NodeVisitor *visitor) = 0;

    virtual If *asIf() { return nullptr; }
    virtual Send *asSend() { return nullptr; }
    virtual Invoke *asInvoke() { return nullptr; }
    virtual Script *asScript() { return nullptr; }
    virtual State *asState() { return nullptr; }
    virtual Transition *asTransition() { return nullptr; }
    virtual HistoryState *asHistoryState() { return nullptr; }
    virtual Scxml *asScxml() { return nullptr; }

private:
    Q_DISABLE_COPY(Node)
};

struct DataElement: public Node
{
    QString id;
    QString src;
    QString expr;
    QString content;

    DataElement(const XmlLocation &xmlLocation): Node(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Param: public Node
{
    QString name;
    QString expr;
    QString location;

    Param(const XmlLocation &xmlLocation): Node(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct DoneData: public Node
{
    QString contents;
    QString expr;
    QVector<Param *> params;

    DoneData(const XmlLocation &xmlLocation): Node(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Instruction: public Node
{
    Instruction(const XmlLocation &xmlLocation): Node(xmlLocation) {}
    virtual ~Instruction() {}
};

typedef QVector<Instruction *> InstructionSequence;
typedef QVector<InstructionSequence *> InstructionSequences;

struct Send: public Instruction
{
    QString event;
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
    QVector<Param *> params;
    QString content;

    Send(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    Send *asSend() Q_DECL_OVERRIDE { return this; }
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Invoke: public Instruction
{
    QString type;
    QString typeexpr;
    QString src;
    QString srcexpr;
    QString id;
    QString idLocation;
    QStringList namelist;
    bool autoforward;
    QVector<Param *> params;
    XmlNode *content;
    InstructionSequence finalize;

    Invoke(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    Invoke *asInvoke() Q_DECL_OVERRIDE { return this; }
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Raise: public Instruction
{
    QString event;

    Raise(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Log: public Instruction
{
    QString label, expr;

    Log(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Script: public Instruction
{
    QString src;
    QString content;

    Script(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    Script *asScript() Q_DECL_OVERRIDE { return this; }
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Assign: public Instruction
{
    QString location;
    QString expr;
    QString content;

    Assign(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct If: public Instruction
{
    QStringList conditions;
    InstructionSequences blocks;

    If(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    If *asIf() Q_DECL_OVERRIDE { return this; }
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Foreach: public Instruction
{
    QString array;
    QString item;
    QString index;
    InstructionSequence block;

    Foreach(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Cancel: public Instruction
{
    QString sendid;
    QString sendidexpr;

    Cancel(const XmlLocation &xmlLocation): Instruction(xmlLocation) {}
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct StateOrTransition: public Node
{
    StateOrTransition(const XmlLocation &xmlLocation): Node(xmlLocation) {}
};

struct StateContainer
{
    StateContainer *parent = nullptr;

    virtual ~StateContainer() {}
    virtual void add(StateOrTransition *s) = 0;
    virtual AbstractState *asAbstractState() { return nullptr; }
    virtual State *asState() { return nullptr; }
    virtual Scxml *asScxml() { return nullptr; }
    Node *asNode() { return dynamic_cast<Node *>(this); }
};

struct AbstractState: public StateContainer
{
    QString id;

    AbstractState *asAbstractState() Q_DECL_OVERRIDE { return this; }
};

struct State: public AbstractState, public StateOrTransition
{
    enum Type { Normal, Parallel, Initial, Final };

    QString initial;
    QVector<DataElement *> dataElements;
    QVector<StateOrTransition *> children;
    InstructionSequences onEntry;
    InstructionSequences onExit;
    DoneData *doneData = 0;
    Type type = Normal;

    AbstractState *initialState = nullptr; // filled during verification

    State(const XmlLocation &xmlLocation): StateOrTransition(xmlLocation) {}

    void add(StateOrTransition *s) Q_DECL_OVERRIDE
    {
        Q_ASSERT(s);
        children.append(s);
    }

    State *asState() Q_DECL_OVERRIDE { return this; }

    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Transition: public StateOrTransition
{
    enum Type { External, Internal };
    QStringList events;
    QScopedPointer<QString> condition;
    QStringList targets;
    InstructionSequence instructionsOnTransition;
    Type type = External;

    QVector<AbstractState *> targetStates; // filled during verification

    Transition(const XmlLocation &xmlLocation): StateOrTransition(xmlLocation) {}

    Transition *asTransition() Q_DECL_OVERRIDE { return this; }

    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct HistoryState: public AbstractState, public StateOrTransition
{
    enum Type { Deep, Shallow };
    Type type = Shallow;
    QVector<StateOrTransition *> children;

    HistoryState(const XmlLocation &xmlLocation): StateOrTransition(xmlLocation) {}
    void add(StateOrTransition *s) Q_DECL_OVERRIDE
    {
        Q_ASSERT(s);
        children.append(s);
    }

    Transition *defaultConfiguration()
    { return children.isEmpty() ? nullptr : children.first()->asTransition(); }

    HistoryState *asHistoryState() Q_DECL_OVERRIDE { return this; }
    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct Scxml: public StateContainer, public Node
{
    enum DataModelType {
        NullDataModel,
        JSDataModel
    };
    enum BindingMethod {
        EarlyBinding,
        LateBinding
    };

    QStringList initial;
    QString name;
    DataModelType dataModel = NullDataModel;
    BindingMethod binding = EarlyBinding;
    QVector<StateOrTransition *> children;
    QVector<DataElement *> dataElements;
    QScopedPointer<Script> script;
    DocumentModel::InstructionSequence initialSetup;

    QVector<AbstractState *> initialStates; // filled during verification

    Scxml(const XmlLocation &xmlLocation): Node(xmlLocation) {}

    void add(StateOrTransition *s) Q_DECL_OVERRIDE
    {
        Q_ASSERT(s);
        children.append(s);
    }

    Scxml *asScxml() { return this; }

    void accept(NodeVisitor *visitor) Q_DECL_OVERRIDE;
};

struct ScxmlDocument
{
    Scxml *root = nullptr;
    QVector<AbstractState *> allStates;
    QVector<Transition *> allTransitions;
    QVector<Node *> allNodes;
    QVector<InstructionSequence *> allSequences;
    bool isVerified = false;

    ~ScxmlDocument()
    {
        delete root;
        qDeleteAll(allNodes);
        qDeleteAll(allSequences);
    }

    State *newState(StateContainer *parent, State::Type type, const XmlLocation &xmlLocation)
    {
        Q_ASSERT(parent);
        State *s = newNode<State>(xmlLocation);
        s->parent = parent;
        s->type = type;
        allStates.append(s);
        parent->add(s);
        return s;
    }

    HistoryState *newHistoryState(StateContainer *parent, const XmlLocation &xmlLocation)
    {
        Q_ASSERT(parent);
        HistoryState *s = newNode<HistoryState>(xmlLocation);
        s->parent = parent;
        allStates.append(s);
        parent->add(s);
        return s;
    }

    Transition *newTransition(StateContainer *parent, const XmlLocation &xmlLocation)
    {
        Transition *t = newNode<Transition>(xmlLocation);
        allTransitions.append(t);
        parent->add(t);
        return t;
    }

    template<typename T>
    T *newNode(const XmlLocation &xmlLocation)
    {
        T *node = new T(xmlLocation);
        allNodes.append(node);
        return node;
    }

    InstructionSequence *newSequence(InstructionSequences *container)
    {
        Q_ASSERT(container);
        InstructionSequence *is = new InstructionSequence;
        allSequences.append(is);
        container->append(is);
        return is;
    }
};

class NodeVisitor
{
public:
    virtual ~NodeVisitor();

    virtual void visit(DataElement *) {}
    virtual void visit(Param *) {}
    virtual bool visit(DoneData *) { return true; }
    virtual void endVisit(DoneData *) {}
    virtual bool visit(Send *) { return true; }
    virtual void endVisit(Send *) {}
    virtual bool visit(Invoke *) { return true; }
    virtual void endVisit(Invoke *) {}
    virtual void visit(Raise *) {}
    virtual void visit(Log *) {}
    virtual void visit(Script *) {}
    virtual void visit(Assign *) {}
    virtual bool visit(If *) { return true; }
    virtual void endVisit(If *) {}
    virtual bool visit(Foreach *) { return true; }
    virtual void endVisit(Foreach *) {}
    virtual void visit(Cancel *) {}
    virtual bool visit(State *) { return true; }
    virtual void endVisit(State *) {}
    virtual bool visit(Transition *) { return true; }
    virtual void endVisit(Transition *) {}
    virtual bool visit(HistoryState *) { return true; }
    virtual void endVisit(HistoryState *) {}
    virtual bool visit(Scxml *) { return true; }
    virtual void endVisit(Scxml *) {}

    void visit(InstructionSequence *sequence)
    {
        Q_ASSERT(sequence);
        Q_FOREACH (Instruction *instruction, *sequence)
            instruction->accept(this);
    }

    void visit(const QVector<DataElement *> &dataElements)
    {
        Q_FOREACH (DataElement *dataElement, dataElements)
            dataElement->accept(this);
    }

    void visit(const QVector<StateOrTransition *> &children)
    {
        Q_FOREACH (StateOrTransition *child, children)
            child->accept(this);
    }

    void visit(const InstructionSequences &sequences)
    {
        Q_FOREACH (InstructionSequence *sequence, sequences)
            visit(sequence);
    }

    void visit(const QVector<Param *> &params)
    {
        Q_FOREACH (Param *param, params)
            param->accept(this);
    }
};

} // DocumentModel namespace

struct ParserState {
    enum Kind {
        Scxml,
        State,
        Parallel,
        Transition,
        Initial,
        Final,
        OnEntry,
        OnExit,
        History,
        Raise,
        If,
        ElseIf,
        Else,
        Foreach,
        Log,
        DataModel,
        Data,
        DataElement,
        Assign,
        DoneData,
        Content,
        Param,
        Script,
        Send,
        Cancel,
        Invoke,
        Finalize,
        None
    };
    Kind kind;
    QString chars;
    DocumentModel::Instruction *instruction;
    DocumentModel::InstructionSequence *instructionContainer;
    QByteArray initialId;

    bool collectChars();

    ParserState(Kind kind=None)
        : kind(kind)
        , instruction(0)
        , instructionContainer(0)
    {}
    ~ParserState() { }

    bool validChild(ParserState::Kind child) const;
    static bool validChild(ParserState::Kind parent, ParserState::Kind child);
    static bool isExecutableContent(ParserState::Kind kind);
};

struct ErrorMessage
{
    enum Severity {
        Debug,
        Info,
        Error
    };
    Severity severity;
    QString msg;
    QString parserState;
    ErrorMessage(Severity severity = Severity::Error,
                 const QString &msg = QStringLiteral("UnknownError"),
                 const QString &parserState = QString())
        : severity(severity), msg(msg), parserState(parserState){ }
    QString severityString() const {
        switch (severity) {
        case Debug:
            return QStringLiteral("Debug: ");
        case Info:
            return QStringLiteral("Info: ");
        case Error:
            return QStringLiteral("Error: ");
        }
        return QStringLiteral("Severity%1: ").arg(severity);
    }
};

struct ParsingOptions {
    ParsingOptions() { }
};

class SCXML_EXPORT ScxmlParser
{
public:
    typedef std::function<QByteArray(const QString &, bool &, ScxmlParser *parser)> LoaderFunction;
    static LoaderFunction loaderForDir(const QString &basedir);

    enum State {
        StartingParsing,
        ParsingScxml,
        ParsingError,
        FinishedParsing,
    };

    ScxmlParser(QXmlStreamReader *xmlReader, LoaderFunction loader = Q_NULLPTR);
    void parse();
    DocumentModel::XmlLocation xmlLocation() const;
    DocumentModel::ScxmlDocument *scxmlDocument();
    StateTable *table();
    void addError(const QString &msg, ErrorMessage::Severity severity = ErrorMessage::Error);
    void addError(const char *msg, ErrorMessage::Severity severity = ErrorMessage::Error);
    void addError(const DocumentModel::XmlLocation &location, const QString &msg);
    std::function<bool(const QString &)> errorDumper() {
        return [this](const QString &msg) -> bool { this->addError(msg); return true; };
    }

    State state() const { return m_state; }
    QList<ErrorMessage> errors() const { return m_errors; }

private:
    bool maybeId(const QXmlStreamAttributes &attributes, QString *id);
    bool checkAttributes(const QXmlStreamAttributes &attributes, const char *attribStr);
    bool checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames,
                         QStringList optionalNames);

    QSet<QString> m_allIds;
    DocumentModel::AbstractState *currentParent() const;

    QScopedPointer<DocumentModel::ScxmlDocument> m_doc;
    DocumentModel::StateContainer *m_currentParent;
    DocumentModel::StateContainer *m_currentState;
    LoaderFunction m_loader;
    QStringList m_namespacesToIgnore;

    QXmlStreamReader *m_reader;
    QVector<ParserState> m_stack;
    State m_state;
    QList<ErrorMessage> m_errors;
    ParsingOptions m_options;
};

} // namespace Scxml

#endif // SCXMLPARSER_H
