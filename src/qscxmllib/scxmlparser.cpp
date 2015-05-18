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

#include "scxmlparser.h"
#include "nulldatamodel.h"
#include "ecmascriptdatamodel.h"
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QState>
#include <QHistoryState>
#include <QEventTransition>
#include <QSignalTransition>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVector>
#include <private/qabstracttransition_p.h>

namespace Scxml {

class ScxmlVerifier: public DocumentModel::NodeVisitor
{
public:
    ScxmlVerifier(std::function<void (const DocumentModel::XmlLocation &, const QString &)> errorHandler)
        : m_errorHandler(errorHandler)
    {}

    bool verify(DocumentModel::ScxmlDocument *doc)
    {
        if (doc->isVerified)
            return true;

        doc->isVerified = true;
        m_doc = doc;
        foreach (DocumentModel::AbstractState *state, doc->allStates) {
            if (state->id.isEmpty()) {
                continue;
#ifndef QT_NO_DEBUG
            } else if (m_stateById.contains(state->id)) {
                Q_ASSERT(!"Should be unreachable: the parser should check for this case!");
#endif // QT_NO_DEBUG
            } else {
                m_stateById[state->id] = state;
            }
        }

        doc->root->accept(this);
        return !m_hasErrors;
    }

private:
    bool visit(DocumentModel::Scxml *scxml) Q_DECL_OVERRIDE
    {
        Q_ASSERT(scxml->initialStates.isEmpty());

        if (scxml->initial.isEmpty()) {
            if (auto firstChild = firstAbstractState(scxml)) {
                scxml->initialStates.append(firstChild);
            }
        } else {
            foreach (const QString &initial, scxml->initial) {
                if (DocumentModel::AbstractState *s = m_stateById.value(initial))
                    scxml->initialStates.append(s);
                else
                    error(scxml->xmlLocation, QStringLiteral("initial state '%1' not found for <scxml> element").arg(initial));
            }
        }

        m_parentNodes.append(scxml);

        return true;
    }

    void endVisit(DocumentModel::Scxml *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::State *state) Q_DECL_OVERRIDE
    {
        Q_ASSERT(state->initialState == nullptr);

        if (state->initial.isEmpty()) {
            state->initialState = firstAbstractState(state);
        } else {
            Q_ASSERT(state->type == DocumentModel::State::Normal);
            if (DocumentModel::AbstractState *s = m_stateById.value(state->initial)) {
                state->initialState = s;
            } else {
                error(state->xmlLocation, QStringLiteral("undefined initial state '%1' for state '%2'").arg(state->initial, state->id));
            }
        }

        switch (state->type) {
        case DocumentModel::State::Normal:
            break;
        case DocumentModel::State::Parallel:
            if (!state->initial.isEmpty()) {
                error(state->xmlLocation, QStringLiteral("parallel states cannot have an initial state"));
            }
            break;
        case DocumentModel::State::Initial:
            if (transitionCount(state) != 1)
                error(state->xmlLocation, QStringLiteral("an initial state can only have one transition, but has '%1'").arg(transitionCount(state)));
            if (DocumentModel::Transition *t = firstTransition(state)) {
                if (!t->events.isEmpty() || !t->condition.isNull()) {
                    error(t->xmlLocation, QStringLiteral("the transition in an initial state cannot have an event or a condition"));
                }
                if (t->targets.isEmpty()) {
                    error(t->xmlLocation, QStringLiteral("the transition in an initial state must have at least one target"));
                }
            }
            foreach (DocumentModel::StateOrTransition *child, state->children) {
                if (DocumentModel::State *s = child->asState()) {
                    error(s->xmlLocation, QStringLiteral("substates are not allowed in initial states"));
                }
            }
            if (parentState() == nullptr) {
                error(state->xmlLocation, QStringLiteral("initial states can only occur in a state"));
            }
            break;
        case DocumentModel::State::Final:
            break;
        default:
            Q_UNREACHABLE();
        }

        m_parentNodes.append(state);
        return true;
    }

    void endVisit(DocumentModel::State *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::Transition *transition) Q_DECL_OVERRIDE
    {
        Q_ASSERT(transition->targetStates.isEmpty());

        if (int size = transition->targets.size())
            transition->targetStates.reserve(size);
        foreach (const QString &target, transition->targets) {
            if (DocumentModel::AbstractState *s = m_stateById.value(target)) {
                if (transition->targetStates.contains(s)) {
                    error(transition->xmlLocation, QStringLiteral("duplicate target '%1'").arg(target));
                } else {
                    transition->targetStates.append(s);
                }
            } else if (!target.isEmpty()) {
                error(transition->xmlLocation, QStringLiteral("unknown state '%1' in target").arg(target));
            }
        }

        m_parentNodes.append(transition);
        return true;
    }

    void endVisit(DocumentModel::Transition *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::HistoryState *state) Q_DECL_OVERRIDE
    {
        bool seenTransition = false;
        foreach (DocumentModel::StateOrTransition *sot, state->children) {
            if (DocumentModel::State *s = sot->asState()) {
                error(s->xmlLocation, QStringLiteral("history state cannot have substates"));
            } else if (DocumentModel::Transition *t = sot->asTransition()) {
                if (seenTransition) {
                    error(t->xmlLocation, QStringLiteral("history state can only have one transition"));
                } else {
                    seenTransition = true;
                    m_parentNodes.append(state);
                    t->accept(this);
                    m_parentNodes.removeLast();
                }
            }
        }

        return false;
    }

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr);
        return true;
    }

    void visit(DocumentModel::Cancel *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
    }

    bool visit(DocumentModel::DoneData *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("donedata"), QStringLiteral("expr"), node->expr);
        return false;
    }

private:
    static int transitionCount(DocumentModel::State *state)
    {
        int count = 0;
        foreach (DocumentModel::StateOrTransition *child, state->children) {
            if (child->asTransition())
                ++count;
        }
        return count;
    }

    static DocumentModel::Transition *firstTransition(DocumentModel::State *state)
    {
        foreach (DocumentModel::StateOrTransition *child, state->children) {
            if (DocumentModel::Transition *t = child->asTransition())
                return t;
        }
        return nullptr;
    }

    static DocumentModel::AbstractState *firstAbstractState(DocumentModel::StateContainer *container)
    {
        QVector<DocumentModel::StateOrTransition *> children;
        if (auto state = container->asState())
            children = state->children;
        else if (auto scxml = container->asScxml())
            children = scxml->children;
        else
            Q_UNREACHABLE();
        foreach (DocumentModel::StateOrTransition *child, children) {
            if (DocumentModel::State *s = child->asState())
                return s;
            else if (DocumentModel::HistoryState *h = child->asHistoryState())
                return h;
        }
        return nullptr;
    }

    void checkExpr(const DocumentModel::XmlLocation &loc, const QString &tag, const QString &attrName, const QString &attrValue)
    {
        if (m_doc->root->dataModel == DocumentModel::Scxml::NullDataModel && !attrValue.isEmpty()) {
            error(loc, QStringLiteral("%1 in <%2> cannot be used with data model 'null'").arg(attrName, tag));
        }
    }

    void error(const DocumentModel::XmlLocation &location, const QString &message)
    {
        m_hasErrors = true;
        if (m_errorHandler)
            m_errorHandler(location, message);
    }

    DocumentModel::Node *parentState() const
    {
        for (int i = m_parentNodes.size() - 1; i >= 0; --i) {
            if (DocumentModel::State *s = m_parentNodes.at(i)->asState())
                return s;
        }

        return nullptr;
    }

private:
    std::function<void (const DocumentModel::XmlLocation &, const QString &)> m_errorHandler;
    DocumentModel::ScxmlDocument *m_doc;
    bool m_hasErrors = false;
    QHash<QString, DocumentModel::AbstractState *> m_stateById;
    QVector<DocumentModel::Node *> m_parentNodes;
};

class StateTableBuilder: public DocumentModel::NodeVisitor
{
    StateTable *m_table = nullptr;

public:
    StateTable *build(DocumentModel::ScxmlDocument *doc)
    {
        m_table = nullptr;
        m_parents.reserve(32);
        m_allTransitions.reserve(doc->allTransitions.size());
        m_docStatesToQStates.reserve(doc->allStates.size());

        doc->root->accept(this);
        wireTransitions();
        applyInitialStates();

        m_parents.clear();
        m_allTransitions.clear();
        m_docStatesToQStates.clear();
        m_currentInstructionSequence = nullptr;
        m_currentTransition = nullptr;

        return m_table;
    }

private:
    using NodeVisitor::visit;

    bool visit(DocumentModel::Scxml *scxml) Q_DECL_OVERRIDE
    {
        m_table = new StateTable;

        switch (scxml->dataModel) {
        case DocumentModel::Scxml::NullDataModel:
            m_table->setDataModel(new NullDataModel(m_table));
            break;
        case DocumentModel::Scxml::JSDataModel:
            m_table->setDataModel(new EcmaScriptDataModel(m_table));
            break;
        default:
            Q_UNREACHABLE();
        }

        switch (scxml->binding) {
        case DocumentModel::Scxml::EarlyBinding:
            m_table->setDataBinding(StateTable::EarlyBinding);
            break;
        case DocumentModel::Scxml::LateBinding:
            m_table->setDataBinding(StateTable::LateBinding);
            break;
        default:
            Q_UNREACHABLE();
        }

        m_table->_name = scxml->name;

        m_parents.append(m_table);
        visit(scxml->children);
        visit(scxml->dataElements);

        m_currentInstructionSequence = &m_table->m_initialSetup;
        if (scxml->script) {
            scxml->script->accept(this);
        }
        visit(&scxml->initialSetup);
        m_currentInstructionSequence = 0;

        m_parents.removeLast();

        foreach (auto initialState, scxml->initialStates) {
            Q_ASSERT(initialState);
            m_initialStates.append(qMakePair(m_table, initialState));
        }

        return false;
    }

    bool visit(DocumentModel::State *state) Q_DECL_OVERRIDE
    {
        QAbstractState *newState = nullptr;
        ExecutableContent::InstructionSequences *onEntry = nullptr, *onExit = nullptr;
        switch (state->type) {
        case DocumentModel::State::Normal: {
            auto s = new ScxmlState(currentParent());
            onEntry = &s->onEntryInstructions;
            onExit = &s->onExitInstructions;
            newState = s;
            if (state->initialState)
                m_initialStates.append(qMakePair(s, state->initialState));
        } break;
        case DocumentModel::State::Parallel: {
            auto s = new ScxmlState(currentParent());
            onEntry = &s->onEntryInstructions;
            onExit = &s->onExitInstructions;
            s->setChildMode(QState::ParallelStates);
            newState = s;
        } break;
        case DocumentModel::State::Initial: {
            auto s = new ScxmlState(currentParent());
            onEntry = &s->onEntryInstructions;
            onExit = &s->onExitInstructions;
            currentParent()->setInitialState(s);
            newState = s;
        } break;
        case DocumentModel::State::Final: {
            auto s = new ScxmlFinalState(currentParent());
            onEntry = &s->onEntryInstructions;
            onExit = &s->onExitInstructions;
            newState = s;
        } break;
        default:
            Q_UNREACHABLE();
        }

        newState->setObjectName(state->id);

        m_docStatesToQStates.insert(state, newState);
        m_parents.append(newState);

        visit(state->dataElements);
        visit(state->children);
        if (onEntry)
            generate(onEntry, state->onEntry);
        if (onExit)
            generate(onExit, state->onExit);
        if (state->doneData)
            state->doneData->accept(this);

        m_parents.removeLast();
        return false;
    }

    bool visit(DocumentModel::Transition *transition) Q_DECL_OVERRIDE
    {
        QState *parentState = 0;
        if (QHistoryState *parent = qobject_cast<QHistoryState*>(m_parents.last())) {
            // QHistoryState cannot have an initial transition, only an initial state.
            // So, work around that by creating an initial state, and add the transition to that.
            parentState = new ScxmlState(parent->parentState());
            parent->setDefaultState(parentState);
        } else {
            parentState = currentParent();
        }
        DataModel::EvaluatorId cond = DataModel::NoEvaluator;
        if (transition->condition) {
            cond = createEvaluatorBool(QStringLiteral("transition"), QStringLiteral("cond"), *transition->condition.data());
        }
        auto newTransition = new ScxmlTransition(parentState,
                                                 toUtf8(transition->events),
                                                 cond);
        parentState->addTransition(newTransition);
        switch (transition->type) {
        case DocumentModel::Transition::External:
            newTransition->setTransitionType(QAbstractTransition::ExternalTransition);
            break;
        case DocumentModel::Transition::Internal:
            newTransition->setTransitionType(QAbstractTransition::InternalTransition);
            break;
        default:
            Q_UNREACHABLE();
        }

        m_currentTransition = newTransition;
        m_allTransitions.insert(newTransition, transition);
        m_currentInstructionSequence = &newTransition->instructionsOnTransition;
        return true;
    }

    void endVisit(DocumentModel::Transition *) Q_DECL_OVERRIDE
    {
        m_currentInstructionSequence = 0;
        m_currentTransition = 0;
    }

    bool visit(DocumentModel::HistoryState *state) Q_DECL_OVERRIDE
    {
        QHistoryState *newState = new QHistoryState(currentParent());
        switch (state->type) {
        case DocumentModel::HistoryState::Shallow:
            newState->setHistoryType(QHistoryState::ShallowHistory);
            break;
        case DocumentModel::HistoryState::Deep:
            newState->setHistoryType(QHistoryState::DeepHistory);
            break;
        default:
            Q_UNREACHABLE();
        }

        newState->setObjectName(state->id);
        m_docStatesToQStates.insert(state, newState);
        m_parents.append(newState);
        return true;
    }

    void endVisit(DocumentModel::HistoryState *) Q_DECL_OVERRIDE
    {
        m_parents.removeLast();
    }

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE
    {
        auto instr = new ExecutableContent::Send;
        instr->instructionLocation = createContext(QStringLiteral("send"));
        instr->event = node->event.toUtf8();
        instr->eventexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr);
        instr->type = node->type;
        instr->typeexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("typeexpr"), node->typeexpr);
        instr->target = node->target;
        instr->targetexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("targetexpr"), node->targetexpr);
        instr->id = node->id;
        instr->idLocation = node->idLocation;
        instr->delay = node->delay;
        instr->delayexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("delayexpr"), node->delayexpr);
        instr->namelist = node->namelist;
        instr->params = create(node->params);
        instr->content = node->content;
        add(instr);
        return false;
    }

    void visit(DocumentModel::Raise *node) Q_DECL_OVERRIDE
    {
        add(new ExecutableContent::Raise(node->event.toUtf8()));
    }

    void visit(DocumentModel::Log *node) Q_DECL_OVERRIDE
    {
        auto expr = createEvaluatorString(QStringLiteral("log"), QStringLiteral("expr"), node->expr);
        auto instr = new ExecutableContent::Log(node->label, expr);
        add(instr);
    }

    void visit(DocumentModel::DataElement *data) Q_DECL_OVERRIDE
    {
        QState *parent = currentParent();
        if (!parent)
            parent = m_table;
        m_table->dataModel()->addData(DataModel::Data(data->id, data->src, data->expr, parent));
    }

    void visit(DocumentModel::Script *node) Q_DECL_OVERRIDE
    {
        auto ctxt = createContext(QStringLiteral("script"), QStringLiteral("source"), node->content);
        auto func = m_table->dataModel()->createScriptEvaluator(node->content, ctxt);
        add(new ExecutableContent::JavaScript(func));
    }

    void visit(DocumentModel::Assign *node) Q_DECL_OVERRIDE
    {
        auto ctxt = createContext(QStringLiteral("assign"), QStringLiteral("expr"), node->expr);
        auto expr = m_table->dataModel()->createAssignmentEvaluator(node->location, node->expr, ctxt);
        add(new ExecutableContent::AssignExpression(expr));
    }

    bool visit(DocumentModel::If *node) Q_DECL_OVERRIDE
    {
        QVector<DataModel::EvaluatorId> conditions(node->conditions.size());
        QString tag = QStringLiteral("if");
        for (int i = 0, ei = node->conditions.size(); i != ei; ++i) {
            conditions[i] = createEvaluatorBool(tag, QStringLiteral("cond"), node->conditions.at(i));
            if (i == 0) {
                tag = QStringLiteral("elif");
            }
        }
        ExecutableContent::InstructionSequences *blocks = new ExecutableContent::InstructionSequences;
        generate(blocks, node->blocks);
        add(new ExecutableContent::If(conditions, blocks));
        return false;
    }

    bool visit(DocumentModel::Foreach *node) Q_DECL_OVERRIDE
    {
        auto ctxt = createContext(QStringLiteral("foreach"));
        auto it = m_table->dataModel()->createForeachEvaluator(node->array, node->item, node->index, ctxt);
        ExecutableContent::InstructionSequence *previous = m_currentInstructionSequence;
        ExecutableContent::InstructionSequence seq;
        m_currentInstructionSequence = &seq;
        visit(&node->block);
        m_currentInstructionSequence = previous;
        add(new ExecutableContent::Foreach(it, seq));
        return false;
    }

    void visit(DocumentModel::Cancel *node) Q_DECL_OVERRIDE
    {
        auto sendidexpr = createEvaluatorString(QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
        add(new ExecutableContent::Cancel(node->sendid.toUtf8(), sendidexpr));
    }

    bool visit(DocumentModel::Invoke *) Q_DECL_OVERRIDE
    {
        Q_UNIMPLEMENTED();
        return false;
    }

    bool visit(DocumentModel::DoneData *node) Q_DECL_OVERRIDE
    {
        auto finalState = qobject_cast<ScxmlFinalState *>(m_parents.last());
        Q_ASSERT(finalState);
        ExecutableContent::DoneData dd(node->contents,
                                       createEvaluatorString(QStringLiteral("donedata"), QStringLiteral("expr"), node->expr),
                                       create(node->params));
        finalState->setDoneData(dd);
        return false;
    }

private: // Utility methods
    static QList<QByteArray> toUtf8(const QStringList &l)
    {
        QList<QByteArray> res;
        foreach (const QString &s, l)
            res.append(s.toUtf8());
        return res;
    }

    QState *currentParent() const
    {
        if (m_parents.isEmpty())
            return nullptr;

        QState *parent = qobject_cast<QState*>(m_parents.last());
        Q_ASSERT(parent);
        return parent;
    }

    void wireTransitions()
    {
        for (QHash<QAbstractTransition *, DocumentModel::Transition*>::const_iterator i = m_allTransitions.begin(), ei = m_allTransitions.end(); i != ei; ++i) {
            QList<QAbstractState *> targets;
            targets.reserve(i.value()->targets.size());
            foreach (DocumentModel::AbstractState *targetState, i.value()->targetStates) {
                QAbstractState *target = m_docStatesToQStates.value(targetState);
                Q_ASSERT(target);
                targets.append(target);
            }
            i.key()->setTargetStates(targets);
        }
    }

    void applyInitialStates()
    {
        foreach (const auto &init, m_initialStates) {
            Q_ASSERT(init.second);
            auto initialState = m_docStatesToQStates.value(init.second);
            Q_ASSERT(initialState);
            init.first->setInitialState(initialState);
        }
    }

    void add(ExecutableContent::Instruction *instr) const
    {
        Q_ASSERT(m_currentInstructionSequence);
        m_currentInstructionSequence->statements.append(ExecutableContent::Instruction::Ptr(instr));
    }

    QVector<ExecutableContent::Param> create(const QVector<DocumentModel::Param *> &from) const
    {
        QVector<ExecutableContent::Param> to;
        to.reserve(from.size());
        foreach (DocumentModel::Param *f, from) {
            ExecutableContent::Param param(
                        f->name,
                        createEvaluatorVariant(QStringLiteral("param"), QStringLiteral("expr"), f->expr),
                        f->location);
            to.append(param);
        }
        return to;
    }

    void generate(ExecutableContent::InstructionSequences *outSequences, const DocumentModel::InstructionSequences &inSequences)
    {
        ExecutableContent::InstructionSequence *previous = m_currentInstructionSequence;
        foreach (DocumentModel::InstructionSequence *sequence, inSequences) {
            m_currentInstructionSequence = outSequences->newInstructions();
            visit(sequence);
        }
        m_currentInstructionSequence = previous;
    }

    QString createContext(const QString &instrName) const
    {
        if (m_currentTransition) {
            QString state;
            if (QState *s = m_currentTransition->sourceState()) {
                state = QStringLiteral(" of state '%1'").arg(s->objectName());
            }
            return QStringLiteral("%1 instruction in transition %2 %3").arg(instrName, m_currentTransition->objectName(), state);
        } else {
            return QStringLiteral("%1 instruction in state %2").arg(instrName, m_parents.last()->objectName());
        }
    }

    QString createContext(const QString &instrName, const QString &attrName, const QString &attrValue) const
    {
        QString location = createContext(instrName);
        return QStringLiteral("%1 with %2=\"%3\"").arg(location, attrName, attrValue);
    }

    DataModel::EvaluatorId createEvaluatorString(const QString &instrName, const QString &attrName, const QString &expr) const
    {
        if (!expr.isEmpty()) {
            QString loc = createContext(instrName, attrName, expr);
            return m_table->dataModel()->createToStringEvaluator(expr, loc);
        }

        return DataModel::NoEvaluator;
    }

    DataModel::EvaluatorId createEvaluatorBool(const QString &instrName, const QString &attrName, const QString &cond) const
    {
        if (!cond.isEmpty()) {
            QString loc = createContext(instrName, attrName, cond);
            return m_table->dataModel()->createToBoolEvaluator(cond, loc);
        }

        return DataModel::NoEvaluator;
    }

    DataModel::EvaluatorId createEvaluatorVariant(const QString &instrName, const QString &attrName, const QString &cond) const
    {
        if (!cond.isEmpty()) {
            QString loc = createContext(instrName, attrName, cond);
            return m_table->dataModel()->createToVariantEvaluator(cond, loc);
        }

        return DataModel::NoEvaluator;
    }

private:
    QVector<QAbstractState *> m_parents;
    QHash<QAbstractTransition *, DocumentModel::Transition*> m_allTransitions;
    QHash<DocumentModel::AbstractState *, QAbstractState *> m_docStatesToQStates;
    QAbstractTransition *m_currentTransition = nullptr;
    ExecutableContent::InstructionSequence *m_currentInstructionSequence = nullptr;
    QVector<QPair<QState *, DocumentModel::AbstractState *>> m_initialStates;
};

ScxmlParser::ScxmlParser(QXmlStreamReader *reader, LoaderFunction loader)
    : m_currentParent(0)
    , m_currentState(0)
    , m_loader(loader)
    , m_reader(reader)
    , m_state(StartingParsing)
{ }

QString ScxmlParser::fileName() const
{
    return m_fileName;
}

void ScxmlParser::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

DocumentModel::AbstractState *ScxmlParser::currentParent() const
{
    DocumentModel::AbstractState *parent = m_currentParent->asAbstractState();
    Q_ASSERT(!m_currentParent || parent);
    return parent;
}

void ScxmlParser::parse()
{
    m_doc.reset(new DocumentModel::ScxmlDocument);
    m_currentParent = m_doc->root;
    m_currentState = m_doc->root;
    while (!m_reader->atEnd()) {
        QXmlStreamReader::TokenType tt = m_reader->readNext();
        switch (tt) {
        case QXmlStreamReader::NoToken:
            // The reader has not yet read anything.
            continue;
        case QXmlStreamReader::Invalid:
            // An error has occurred, reported in error() and errorString().
            break;
        case QXmlStreamReader::StartDocument:
            // The reader reports the XML version number in documentVersion(), and the encoding
            // as specified in the XML document in documentEncoding(). If the document is declared
            // standalone, isStandaloneDocument() returns true; otherwise it returns false.
            break;
        case QXmlStreamReader::EndDocument:
            // The reader reports the end of the document.
            if (!m_stack.isEmpty() || m_state != FinishedParsing) {
                addError(QStringLiteral("document finished without a proper scxml item"));
                m_state = ParsingError;
            }
            break;
        case QXmlStreamReader::StartElement:
            // The reader reports the start of an element with namespaceUri() and name(). Empty
            // elements are also reported as StartElement, followed directly by EndElement.
            // The convenience function readElementText() can be called to concatenate all content
            // until the corresponding EndElement. Attributes are reported in attributes(),
            // namespace declarations in namespaceDeclarations().
        {
            QStringRef elName = m_reader->name();
            QXmlStreamAttributes attributes = m_reader->attributes();
            if (!m_stack.isEmpty() && (m_stack.last().kind == ParserState::DataElement
                                       || m_stack.last().kind == ParserState::Data)) {
                /*switch (m_table->dataModel()) {
                case StateTable::None:
                    break; // error?
                case StateTable::Json:
                case StateTable::Javascript:
                {
                    ParserState pNew = ParserState(ParserState::DataElement);
                    QJsonObject obj;
                    foreach (const QXmlStreamAttribute &attribute, attributes)
                        obj.insert(QStringLiteral("@").append(attribute.name()), attribute.value().toString());
                    pNew.jsonValue = obj;
                    m_stack.append(pNew);
                    break;
                }
                case StateTable::Xml:
                {
                    ParserState pNew = ParserState(ParserState::DataElement);
                    Q_ASSERT(0); // to do, use Scxml::XmlNode
                }
                }*/
                break;
            } else if (elName == QLatin1String("scxml")) {
                m_doc->root = new DocumentModel::Scxml(xmlLocation());
                m_doc->root->xmlLocation = xmlLocation();
                auto scxml = m_doc->root;
                if (m_state != StartingParsing || !m_stack.isEmpty()) {
                    addError(xmlLocation(), QStringLiteral("found scxml tag mid stream"));
                    m_state = ParsingError;
                    return;
                } else {
                    m_state = ParsingScxml;
                }
                if (!checkAttributes(attributes, "version|initial,datamodel,binding,name")) return;
                if (m_reader->namespaceUri() != QLatin1String("http://www.w3.org/2005/07/scxml")) {
                    addError(QStringLiteral("default namespace must be set with xmlns=\"http://www.w3.org/2005/07/scxml\" in the scxml tag"));
                    return;
                }
                if (attributes.value(QLatin1String("version")) != QLatin1String("1.0")) {
                    addError(QStringLiteral("unsupported scxml version, expected 1.0 in scxml tag"));
                    return;
                }
                ParserState pNew = ParserState(ParserState::Scxml);
                pNew.initialId = attributes.value(QLatin1String("initial")).toUtf8();
                QStringRef datamodel = attributes.value(QLatin1String("datamodel"));
                if (datamodel.isEmpty() || datamodel == QLatin1String("null")) {
                    scxml->dataModel = DocumentModel::Scxml::NullDataModel;
                } else if (datamodel == QLatin1String("ecmascript")) {
                    scxml->dataModel = DocumentModel::Scxml::JSDataModel;
                } else {
                    addError(QStringLiteral("Unsupported data model '%1' in scxml")
                             .arg(datamodel.toString()));
                }
                QStringRef binding = attributes.value(QLatin1String("binding"));
                if (binding.isEmpty() || binding == QLatin1String("early")) {
                    scxml->binding = DocumentModel::Scxml::EarlyBinding;
                } else if (binding == QLatin1String("late")) {
                    scxml->binding = DocumentModel::Scxml::LateBinding;
                } else {
                    addError(QStringLiteral("Unsupperted binding type '%1'")
                             .arg(binding.toString()));
                    return;
                }
                QStringRef name = attributes.value(QLatin1String("name"));
                if (!name.isEmpty()) {
                    scxml->name = name.toString();
                }
                m_currentState = m_currentParent = m_doc->root;
                pNew.instructionContainer = &m_doc->root->initialSetup;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("state")) {
                if (!checkAttributes(attributes, "|id,initial")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Normal, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                ParserState pNew = ParserState(ParserState::State);
                pNew.initialId = attributes.value(QLatin1String("initial")).toUtf8();
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("parallel")) {
                if (!checkAttributes(attributes, "|id")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Parallel, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                m_currentState = m_currentParent = newState;
                m_stack.append(ParserState(ParserState::Parallel));
            } else if (elName == QLatin1String("initial")) {
                if (!checkAttributes(attributes, "")) return;
                if (currentParent()->asState()->type == DocumentModel::State::Parallel) {
                    addError(QStringLiteral("Explicit initial state for parallel states not supported (only implicitly through the initial states of its substates)"));
                    m_state = ParsingError;
                    return;
                }
                ParserState pNew(ParserState::Initial);
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Initial, xmlLocation());
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("transition")) {
                if (!checkAttributes(attributes, "|event,cond,target,type")) return;
                auto transition = m_doc->newTransition(m_currentParent, xmlLocation());
                transition->events = attributes.value(QLatin1String("event")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                transition->targets = attributes.value(QLatin1String("target")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                if (attributes.hasAttribute(QStringLiteral("cond")))
                    transition->condition.reset(new QString(attributes.value(QLatin1String("cond")).toString()));
                QStringRef type = attributes.value(QLatin1String("type"));
                if (type.isEmpty() || type == QLatin1String("external")) {
                    transition->type = DocumentModel::Transition::External;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
                } else if (type == QLatin1String("internal")) {
                    transition->type = DocumentModel::Transition::Internal;
#endif
                } else {
                    addError(QStringLiteral("invalid transition type '%1', valid values are 'external' and 'internal'").arg(type.toString()));
                    m_state = ParsingError;
                    break;
                }
                ParserState pNew = ParserState(ParserState::Transition);
                pNew.instructionContainer = &transition->instructionsOnTransition;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("final")) {
                if (!checkAttributes(attributes, "|id")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Final, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                m_currentState = m_currentParent = newState;
                m_stack.append(ParserState(ParserState::Final));
            } else if (elName == QLatin1String("history")) {
                if (!checkAttributes(attributes, "|id,type")) return;
                auto newState = m_doc->newHistoryState(currentParent(), xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                QStringRef type = attributes.value(QLatin1String("type"));
                if (type.isEmpty() || type == QLatin1String("shallow")) {
                    newState->type = DocumentModel::HistoryState::Shallow;
                } else if (type == QLatin1String("deep")) {
                    newState->type = DocumentModel::HistoryState::Deep;
                } else {
                    addError(QStringLiteral("invalid history type %1, valid values are 'shallow' and 'deep'").arg(type.toString()));
                    m_state = ParsingError;
                    return;
                }
                ParserState pNew = ParserState(ParserState::History);
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("onentry")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnEntry);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                case ParserState::State:
                case ParserState::Parallel:
                    if (DocumentModel::State *s = m_currentState->asState()) {
                        pNew.instructionContainer = m_doc->newSequence(&s->onEntry);
                        break;
                    }
                    // intentional fall-through
                default:
                    addError(QStringLiteral("unexpected container state for onentry"));
                    m_state = ParsingError;
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("onexit")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnExit);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                case ParserState::State:
                case ParserState::Parallel:
                    if (DocumentModel::State *s = m_currentState->asState()) {
                        pNew.instructionContainer = m_doc->newSequence(&s->onExit);
                        break;
                    }
                    // intentional fall-through
                default:
                    addError(QStringLiteral("unexpected container state for onexit"));
                    m_state = ParsingError;
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("raise")) {
                if (!checkAttributes(attributes, "event")) return;
                ParserState pNew = ParserState(ParserState::Raise);
                auto raise = m_doc->newNode<DocumentModel::Raise>(xmlLocation());
                raise->event = attributes.value(QLatin1String("event")).toString();
                pNew.instruction = raise;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("if")) {
                if (!checkAttributes(attributes, "cond")) return;
                ParserState pNew = ParserState(ParserState::If);
                auto *ifI = m_doc->newNode<DocumentModel::If>(xmlLocation());
                pNew.instruction = ifI;
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                pNew.instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("elseif")) {
                if (!checkAttributes(attributes, "cond")) return;
                DocumentModel::If *ifI = m_stack.last().instruction->asIf();
                Q_ASSERT(ifI);
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                m_stack.last().instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(ParserState(ParserState::ElseIf));
            } else if (elName == QLatin1String("else")) {
                if (!checkAttributes(attributes, "")) return;
                DocumentModel::If *ifI = m_stack.last().instruction->asIf();
                Q_ASSERT(ifI);
                m_stack.last().instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(ParserState(ParserState::Else));
            } else if (elName == QLatin1String("foreach")) {
                if (!checkAttributes(attributes, "array,item|index")) return;
                ParserState pNew = ParserState(ParserState::Foreach);
                auto foreachI = m_doc->newNode<DocumentModel::Foreach>(xmlLocation());
                foreachI->array = attributes.value(QLatin1String("array")).toString();
                foreachI->item = attributes.value(QLatin1String("item")).toString();
                foreachI->index = attributes.value(QLatin1String("index")).toString();
                pNew.instruction = foreachI;
                pNew.instructionContainer = &foreachI->block;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("log")) {
                if (!checkAttributes(attributes, "|label,expr")) return;
                ParserState pNew = ParserState(ParserState::Log);
                auto logI = m_doc->newNode<DocumentModel::Log>(xmlLocation());
                logI->label = attributes.value(QLatin1String("label")).toString();
                logI->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = logI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("datamodel")) {
                if (!checkAttributes(attributes, "")) return;
                m_stack.append(ParserState(ParserState::DataModel));
            } else if (elName == QLatin1String("data")) {
                if (!checkAttributes(attributes, "id|src,expr")) return;
                auto data = m_doc->newNode<DocumentModel::DataElement>(xmlLocation());
                data->id = attributes.value(QLatin1String("id")).toString();
                data->src = attributes.value(QLatin1String("src")).toString();
                data->expr = attributes.value(QLatin1String("expr")).toString();
                if (!data->src.isEmpty()) {
                    addError(QStringLiteral("the source attribute in a data tag is unsupported")); // FIXME: use a loader like in <script>
                }
                if (DocumentModel::Scxml *scxml = m_currentParent->asScxml()) {
                    scxml->dataElements.append(data);
                } else if (DocumentModel::State *state = m_currentParent->asState()) {
                    state->dataElements.append(data);
                } else {
                    Q_UNREACHABLE();
                }
                m_stack.append(ParserState(ParserState::Data));
            } else if (elName == QLatin1String("assign")) {
                if (!checkAttributes(attributes, "location|expr")) return;
                ParserState pNew = ParserState(ParserState::Assign);
                auto assign = m_doc->newNode<DocumentModel::Assign>(xmlLocation());
                assign->location = attributes.value(QLatin1String("location")).toString();
                assign->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = assign;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("donedata")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew = ParserState(ParserState::DoneData);
                m_stack.append(pNew);
                bool handled = false;
                if (DocumentModel::State *s = m_currentState->asState()) {
                    if (s->type == DocumentModel::State::Final) {
                        handled = true;
                        if (s->doneData) {
                            addError(QLatin1String("state can only have one donedata"));
                            m_state = ParsingError;
                        } else {
                            s->doneData = m_doc->newNode<DocumentModel::DoneData>(xmlLocation());
                        }
                    }
                }
                if (!handled) {
                    addError(QStringLiteral("donedata can only occur in a final state"));
                    m_state = ParsingError;
                }
            } else if (elName == QLatin1String("content")) {
                if (!checkAttributes(attributes, "|expr")) return;
                switch (m_stack.last().kind) {
                case ParserState::DoneData: {
                    DocumentModel::State *s = m_currentState->asState();
                    Q_ASSERT(s);
                    s->doneData->expr = attributes.value(QLatin1String("expr")).toString();
                } break;
                case ParserState::Send: {
                    DocumentModel::Send *s = m_stack.last().instruction->asSend();
                    Q_ASSERT(s);
                    s->content = attributes.value(QLatin1String("expr")).toString();
                } break;
                default:
                    addError(QStringLiteral("unexpected parent of content %1").arg(m_stack.last().kind));
                    m_state = ParsingError;
                }
                ParserState pNew = ParserState(ParserState::Content);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("param")) {
                if (!checkAttributes(attributes, "name|expr,location")) return;
                ParserState pNew = ParserState(ParserState::Param);
                auto param = m_doc->newNode<DocumentModel::Param>(xmlLocation());
                param->name = attributes.value(QLatin1String("name")).toString();
                param->expr = attributes.value(QLatin1String("expr")).toString();
                param->location = attributes.value(QLatin1String("location")).toString();
                switch (m_stack.last().kind) {
                case ParserState::DoneData: {
                    DocumentModel::State *s = m_currentState->asState();
                    Q_ASSERT(s);
                    Q_ASSERT(s->doneData);
                    s->doneData->params.append(param);
                } break;
                case ParserState::Send: {
                    DocumentModel::Send *s = m_stack.last().instruction->asSend();
                    Q_ASSERT(s);
                    s->params.append(param);
                } break;
                case ParserState::Invoke: {
                    DocumentModel::Invoke *i = m_stack.last().instruction->asInvoke();
                    Q_ASSERT(i);
                    i->params.append(param);
                } break;
                default:
                    addError(QStringLiteral("unexpected parent of param %1").arg(m_stack.last().kind));
                    m_state = ParsingError;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("script")) {
                if (!checkAttributes(attributes, "|src")) return;
                ParserState pNew = ParserState(ParserState::Script);
                auto *script = m_doc->newNode<DocumentModel::Script>(xmlLocation());
                script->src = attributes.value(QLatin1String("src")).toString();
                pNew.instruction = script;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("send")) {
                if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr,target,targetexpr")) return;
                ParserState pNew = ParserState(ParserState::Send);
                auto *send = m_doc->newNode<DocumentModel::Send>(xmlLocation());
                send->event = attributes.value(QLatin1String("event")).toString();
                send->eventexpr = attributes.value(QLatin1String("eventexpr")).toString();
                send->delay = attributes.value(QLatin1String("delay")).toString();
                send->delayexpr = attributes.value(QLatin1String("delayexpr")).toString();
                send->id = attributes.value(QLatin1String("id")).toString();
                send->idLocation = attributes.value(QLatin1String("idlocation")).toString();
                send->type = attributes.value(QLatin1String("type")).toString();
                send->typeexpr = attributes.value(QLatin1String("typeexpr")).toString();
                send->target = attributes.value(QLatin1String("target")).toString();
                send->targetexpr = attributes.value(QLatin1String("targetexpr")).toString();
                if (attributes.hasAttribute(QLatin1String("namelist")))
                    send->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                pNew.instruction = send;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("cancel")) {
                if (!checkAttributes(attributes, "|sendid,sendidexpr")) return;
                ParserState pNew = ParserState(ParserState::Cancel);
                auto *cancel = m_doc->newNode<DocumentModel::Cancel>(xmlLocation());
                cancel->sendid = attributes.value(QLatin1String("sendid")).toString();
                cancel->sendidexpr = attributes.value(QLatin1String("sendidexpr")).toString();
                pNew.instruction = cancel;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("invoke")) {
                if (true) {
                    addError(xmlLocation(), QStringLiteral("<invoke> is not supported"));
                    m_state = ParsingError;
                } else {
                    if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr")) return;
                    ParserState pNew = ParserState(ParserState::Invoke);
                    auto *invoke = m_doc->newNode<DocumentModel::Invoke>(xmlLocation());
                    invoke->src = attributes.value(QLatin1String("src")).toString();
                    invoke->srcexpr = attributes.value(QLatin1String("srcexpr")).toString();
                    invoke->id = attributes.value(QLatin1String("id")).toString();
                    invoke->idLocation = attributes.value(QLatin1String("idlocation")).toString();
                    invoke->type = attributes.value(QLatin1String("type")).toString();
                    invoke->typeexpr = attributes.value(QLatin1String("typeexpr")).toString();
                    QStringRef autoforwardS = attributes.value(QLatin1String("autoforward"));
                    if (QStringRef::compare(autoforwardS, QLatin1String("true"), Qt::CaseInsensitive) == 0
                            || QStringRef::compare(autoforwardS, QLatin1String("yes"), Qt::CaseInsensitive) == 0
                            || QStringRef::compare(autoforwardS, QLatin1String("t"), Qt::CaseInsensitive) == 0
                            || QStringRef::compare(autoforwardS, QLatin1String("y"), Qt::CaseInsensitive) == 0
                            || autoforwardS == QLatin1String("1"))
                        invoke->autoforward = true;
                    else
                        invoke->autoforward = false;
                    invoke->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                    pNew.instruction = invoke;
                    m_stack.append(pNew);
                }
            } else if (elName == QLatin1String("finalize")) {
                ParserState pNew(ParserState::Finalize);
                auto invoke = m_stack.last().instruction->asInvoke();
                Q_ASSERT(invoke);
                pNew.instructionContainer = &invoke->finalize;
                m_stack.append(pNew);
            } else {
                addError(QStringLiteral("unexpected element %1").arg(elName.toString()));
            }
            if (m_stack.size()>1 && !m_stack.at(m_stack.size()-2).validChild(m_stack.last().kind)) {
                addError(QStringLiteral("invalid child"));
                m_state = ParsingError;
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            // The reader reports the end of an element with namespaceUri() and name().
        {
            ParserState p = m_stack.last();
            m_stack.removeLast();
            switch (p.kind) {
            case ParserState::Scxml:
                if (m_state == ParsingScxml) {
                    m_state = FinishedParsing;
                } else {
                    m_state = ParsingError;
                }
                return;
            case ParserState::State:
            case ParserState::Parallel:
            case ParserState::Initial:
            case ParserState::Final:
            case ParserState::History:
                Q_ASSERT(m_currentParent->parent);
                m_currentState = m_currentParent = m_currentParent->parent;
                break;
            case ParserState::Transition:
            case ParserState::OnEntry:
            case ParserState::OnExit:
            case ParserState::ElseIf:
            case ParserState::Else:
                break;
            case ParserState::Script:
            {
                DocumentModel::Script *scriptI = p.instruction->asScript();
                if (!p.chars.trimmed().isEmpty()) {
                    scriptI->content = p.chars.trimmed();
                    if (!scriptI->src.isEmpty())
                        addError(QStringLiteral("both scr and source content given to script, will ignore external content"));
                } else if (!scriptI->src.isEmpty()) {
                    if (!m_loader) {
                        addError(QStringLiteral("cannot parse a document with external dependencies without a loader"));
                        m_state = ParsingError;
                    } else {
                        bool ok;
                        QByteArray data = m_loader(scriptI->src, ok, this);
                        if (!ok) {
                            addError(QStringLiteral("failed to load external dependency"));
                            m_state = ParsingError;
                        } else {
                            scriptI->content = QString::fromUtf8(data);
                        }
                    }
                }
            } // very intentional fallthrough!
            case ParserState::Raise:
            case ParserState::If:
            case ParserState::Foreach:
            case ParserState::Log:
            case ParserState::Assign:
            case ParserState::Send:
            case ParserState::Cancel:
            case ParserState::Invoke: {
                DocumentModel::InstructionSequence *instructions = m_stack.last().instructionContainer;
                if (!instructions) {
                    addError(QStringLiteral("got executable content within an element that did not set instructionContainer"));
                    m_state = ParsingError;
                    return;
                }
                instructions->append(p.instruction);
                p.instruction = 0;
                break;
            }
            case ParserState::Finalize:
            case ParserState::DataModel:
            case ParserState::DataElement:
            case ParserState::DoneData:
            case ParserState::Param:
            case ParserState::None:
                break;
            case ParserState::Content:
                if (!p.chars.trimmed().isEmpty()) {
                    Q_ASSERT(!m_stack.isEmpty());
                    switch (m_stack.last().kind) {
                    case ParserState::DoneData: // see test529
                        m_currentState->asState()->doneData->contents = p.chars.simplified();
                        break;
                    case ParserState::Send: // see test179
                        m_stack.last().instruction->asSend()->content = p.chars.simplified();
                        break;
                    default:
                        break;
                    }
                }
                break;
            case ParserState::Data: {
                DocumentModel::DataElement *data = nullptr;
                if (auto state = m_currentParent->asState()) {
                    data = state->dataElements.last();
                } else if (auto scxml = m_currentParent->asNode()->asScxml()) {
                    data = scxml->dataElements.last();
                } else {
                    Q_UNREACHABLE();
                }
                if (!data->src.isEmpty() && !data->expr.isEmpty()) {
                    addError(QStringLiteral("data element with both 'src' and 'expr' attributes"));
                    m_state = ParsingError;
                    return;
                }
                if (!p.chars.trimmed().isEmpty()) {
                    if (!data->src.isEmpty()) {
                        addError(QStringLiteral("data element with both 'src' attribute and CDATA"));
                        m_state = ParsingError;
                        return;
                    } else if (!data->expr.isEmpty()) {
                        addError(QStringLiteral("data element with both 'expr' attribute and CDATA"));
                        m_state = ParsingError;
                        return;
                    } else {
                        data->expr = p.chars.simplified();
                    }
                }
            } break;
            } // parser state
        } // QXmlStreamReader::EndElement
        case QXmlStreamReader::Characters:
            // The reader reports characters in text(). If the characters are all white-space,
            // isWhitespace() returns true. If the characters stem from a CDATA section,
            // isCDATA() returns true.
            if (m_stack.isEmpty())
                break;
            if (m_stack.last().collectChars())
                m_stack.last().chars.append(m_reader->text());
            break;
        case QXmlStreamReader::Comment:
            // The reader reports a comment in text().
            break;
        case QXmlStreamReader::DTD:
            // The reader reports a DTD in text(), notation declarations in notationDeclarations(),
            // and entity declarations in entityDeclarations(). Details of the DTD declaration are
            // reported in in dtdName(), dtdPublicId(), and dtdSystemId().
            break;
        case QXmlStreamReader::EntityReference:
            // The reader reports an entity reference that could not be resolved. The name of
            // the reference is reported in name(), the replacement text in text().
            break;
        case QXmlStreamReader::ProcessingInstruction:
            break;
        }
    }
    if (m_reader->hasError()
            && m_reader->error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        addError(QStringLiteral("Error parsing scxml file"));
        addError(m_reader->errorString());
        m_state = ParsingError;
    }
}

DocumentModel::XmlLocation ScxmlParser::xmlLocation() const
{
    return DocumentModel::XmlLocation(m_reader->lineNumber(), m_reader->columnNumber());
}

DocumentModel::ScxmlDocument *ScxmlParser::scxmlDocument()
{
    if (!m_doc)
        return nullptr;

    auto handler = [this](const DocumentModel::XmlLocation &location, const QString &msg) {
        this->addError(location, msg);
    };

    if (ScxmlVerifier(handler).verify(m_doc.data()))
        return m_doc.data();
    else
        return nullptr;
}

StateTable *ScxmlParser::table()
{
    if (DocumentModel::ScxmlDocument *doc = scxmlDocument())
        return StateTableBuilder().build(doc);
    else
        return nullptr;
}

void ScxmlParser::addError(const QString &msg, ErrorMessage::Severity severity)
{
    m_errors.append(ErrorMessage(m_fileName,
                                 m_reader->lineNumber(),
                                 m_reader->columnNumber(),
                                 severity,
                                 msg));
    if (severity == ErrorMessage::Error)
        m_state = ParsingError;
}

void ScxmlParser::addError(const DocumentModel::XmlLocation &location, const QString &msg)
{
    m_errors.append(ErrorMessage(m_fileName,
                                 location.line,
                                 location.column,
                                 ErrorMessage::Error,
                                 msg));
    m_state = ParsingError;
}

bool ScxmlParser::maybeId(const QXmlStreamAttributes &attributes, QString *id)
{
    Q_ASSERT(id);
    QString idStr = attributes.value(QLatin1String("id")).toString();
    if (!idStr.isEmpty()) {
        if (m_allIds.contains(idStr)) {
            addError(xmlLocation(), QStringLiteral("duplicate id '%1'").arg(idStr));
        } else {
            m_allIds.insert(idStr);
            *id = idStr;
        }
    }
    return true;
}

bool ScxmlParser::checkAttributes(const QXmlStreamAttributes &attributes, const char *attribStr)
{
    QString allAttrib = QString::fromLatin1(attribStr);
    QStringList attrSplit = allAttrib.split(QLatin1Char('|'));
    QStringList requiredNames, optionalNames;
    requiredNames = attrSplit.value(0).split(QLatin1Char(','), QString::SkipEmptyParts);
    optionalNames = attrSplit.value(1).split(QLatin1Char(','), QString::SkipEmptyParts);
    if (attrSplit.size() > 2) {
        addError(QStringLiteral("Internal error, invalid attribStr in checkAttributes"));
        m_state = ParsingError;
    }
    foreach (const QString &rName, requiredNames)
        if (rName.isEmpty())
            requiredNames.removeOne(rName);
    foreach (const QString &oName, optionalNames)
        if (oName.isEmpty())
            optionalNames.removeOne(oName);
    return checkAttributes(attributes, requiredNames, optionalNames);
}

bool ScxmlParser::checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames, QStringList optionalNames)
{
    foreach (const QXmlStreamAttribute &attribute, attributes) {
        QStringRef ns = attribute.namespaceUri();
        if (!ns.isEmpty() && ns != QLatin1String("http://www.w3.org/2005/07/scxml")) {
            foreach (const QString &nsToIgnore, m_namespacesToIgnore) {
                if (ns == nsToIgnore)
                    continue;
            }
            m_namespacesToIgnore << ns.toString();
            addError(QStringLiteral("Ignoring unexpected namespace %1").arg(ns.toString()),
                     ErrorMessage::Info);
            continue;
        }
        const QString name = attribute.name().toString();
        if (!requiredNames.removeOne(name) && !optionalNames.contains(name)) {
            addError(QStringLiteral("Unexpected attribute '%1'").arg(name));
            m_state = ParsingError;
            return false;
        }
    }
    if (!requiredNames.isEmpty()) {
        addError(QStringLiteral("Missing required attributes: '%1'")
                 .arg(requiredNames.join(QLatin1String("', '"))));
        m_state = ParsingError;
        return false;
    }
    return true;
}

ScxmlParser::LoaderFunction ScxmlParser::loaderForDir(const QString &basedir)
{
    return [basedir](const QString &path, bool &ok, ScxmlParser *parser) -> QByteArray {
        ok = false;
        QFileInfo fInfo(path);
        if (fInfo.isRelative())
            fInfo = QFileInfo(QDir(basedir).filePath(path));
        if (!fInfo.exists()) {
            parser->addError(QStringLiteral("src attribute resolves to non existing file (%1)").arg(fInfo.absoluteFilePath()));
        } else {
            QFile f(fInfo.absoluteFilePath());
            if (f.open(QFile::ReadOnly)) {
                ok = true;
                return f.readAll();
            } else {
                parser->addError(QStringLiteral("Failure opening file %1: %2")
                         .arg(fInfo.absoluteFilePath(), f.errorString()));
            }
        }
        return QByteArray();
    };
}

bool Scxml::ParserState::collectChars() {
    switch (kind) {
    case Content:
    case Data:
    case Script:
        return true;
    default:
        break;
    }
    return false;
}

bool ParserState::validChild(ParserState::Kind child) const {
    return validChild(kind, child);
}

bool ParserState::validChild(ParserState::Kind parent, ParserState::Kind child)
{
    switch (parent) {
    case ParserState::Scxml:
        switch (child) {
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::Final:
        case ParserState::DataModel:
        case ParserState::Script:
        case ParserState::Transition:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::State:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::Transition:
        case ParserState::Initial:
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::Final:
        case ParserState::History:
        case ParserState::DataModel:
        case ParserState::Invoke:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::Parallel:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::Transition:
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::History:
        case ParserState::DataModel:
        case ParserState::Invoke:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::Transition:
        return isExecutableContent(child);
    case ParserState::Initial:
        return (child == ParserState::Transition);
    case ParserState::Final:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::DoneData:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::OnEntry:
    case ParserState::OnExit:
        return isExecutableContent(child);
        return false;
    case ParserState::History:
        return (child == ParserState::Transition);
    case ParserState::Raise:
        return false;
    case ParserState::If:
        return (child == ParserState::ElseIf || child == ParserState::Else
                || isExecutableContent(child));
    case ParserState::ElseIf:
    case ParserState::Else:
        return false;
    case ParserState::Foreach:
        return isExecutableContent(child);
    case ParserState::Log:
        return false;
    case ParserState::DataModel:
        return (child == ParserState::Data);
    case ParserState::Data:
        return (child == ParserState::DataElement);
    case ParserState::DataElement:
        return (child == ParserState::DataElement);
    case ParserState::Assign:
        return (child == ParserState::DataElement);
    case ParserState::DoneData:
        return (child == ParserState::Content || child == ParserState::Param);
    case ParserState::Send:
        return (child == ParserState::Param || child == ParserState::Content
                || isExecutableContent(child));
    case ParserState::Content:
    case ParserState::Param:
    case ParserState::Cancel:
    case ParserState::Invoke:
    case ParserState::Finalize:
        return isExecutableContent(child);
        break;
    case ParserState::Script:
    case ParserState::None:
        break;
    }
    return false;
}

bool Scxml::ParserState::isExecutableContent(Scxml::ParserState::Kind kind) {
    switch (kind) {
    case Raise:
    case Send:
    case Log:
    case Script:
    case Assign:
    case If:
    case Foreach:
    case Cancel:
    case Invoke:
        return true;
    default:
        break;
    }
    return false;
}

DocumentModel::Node::~Node()
{
}

void DocumentModel::DataElement::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Param::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::DoneData::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        foreach (Param *param, params)
            param->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Send::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(params);
    }
    visitor->endVisit(this);
}

void DocumentModel::Invoke::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(params);
        visitor->visit(&finalize);
    }
    visitor->endVisit(this);
}

void DocumentModel::Raise::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Log::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Script::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Assign::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::If::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(blocks);
    }
    visitor->endVisit(this);
}

void DocumentModel::Foreach::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(&block);
    }
    visitor->endVisit(this);
}

void DocumentModel::Cancel::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::State::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(dataElements);
        visitor->visit(children);
        visitor->visit(onEntry);
        visitor->visit(onExit);
        if (doneData)
            doneData->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Transition::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(&instructionsOnTransition);
    }
    visitor->endVisit(this);
}

void DocumentModel::HistoryState::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        if (Transition *t = defaultConfiguration())
            t->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Scxml::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(children);
        visitor->visit(dataElements);
        if (script)
            script->accept(visitor);
        visitor->visit(&initialSetup);
    }
    visitor->endVisit(this);
}

DocumentModel::NodeVisitor::~NodeVisitor()
{}

} // namespace Scxml
