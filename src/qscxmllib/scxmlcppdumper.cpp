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

#include "scxmlcppdumper.h"
#include <algorithm>

namespace Scxml {

struct StringListDumper {
    StringListDumper &operator <<(const QString &s) {
        text.append(s);
        return *this;
    }

    StringListDumper &operator <<(const QLatin1String &s) {
        text.append(s);
        return *this;
    }
    StringListDumper &operator <<(const char *s) {
        text.append(QLatin1String(s));
        return *this;
    }
    StringListDumper &operator <<(int i) {
        text.append(QString::number(i));
        return *this;
    }
    StringListDumper &operator <<(const QByteArray &s) {
        text.append(QString::fromUtf8(s));
        return *this;
    }

    bool isEmpty() const {
        return text.isEmpty();
    }

    void write(QTextStream &out, const QString &prefix, const QString &suffix) const
    {
        foreach (const QString &line, text)
            out << prefix << line << suffix;
    }

    QStringList text;
};

struct Method {
    QString name;
    StringListDumper publicDeclaration; // void f(int i = 0);
    StringListDumper implementationHeader; // void f(int i)
    StringListDumper initializer;
    StringListDumper publicImplementationCode; // d->f(i);
    StringListDumper impl; // m_i = ++i;
};

struct MainClass {
    StringListDumper implIncludes;
    QString className;
    StringListDumper classFields;
    Method init;
    Method constructor;
    Method destructor;
    StringListDumper signalMethods;
    QList<Method> publicMethods;
    StringListDumper publicSlotDeclarations;
    StringListDumper publicSlotDefinitions;
    QList<Method> privateMethods;
};

namespace {

QString cEscape(const QString &str)
{
    QString res;
    int lastI = 0;
    for (int i = 0; i < str.length(); ++i) {
        QChar c = str.at(i);
        if (c < QLatin1Char(' ') || c == QLatin1Char('\\') || c == QLatin1Char('\"')) {
            res.append(str.mid(lastI, i - lastI));
            lastI = i + 1;
            if (c == QLatin1Char('\\')) {
                res.append(QLatin1String("\\\\"));
            } else if (c == QLatin1Char('\"')) {
                res.append(QLatin1String("\\\""));
            } else if (c == QLatin1Char('\n')) {
                res.append(QLatin1String("\\n"));
            } else if (c == QLatin1Char('\r')) {
                res.append(QLatin1String("\\r"));
            } else {
                char buf[6];
                ushort cc = c.unicode();
                buf[0] = '\\';
                buf[1] = 'u';
                for (int i = 0; i < 4; ++i) {
                    ushort ccc = cc & 0xF;
                    buf[5 - i] = ccc <= 9 ? '0' + ccc : 'a' + ccc - 10;
                    cc >>= 4;
                }
                res.append(QLatin1String(&buf[0], 6));
            }
        }
    }
    if (lastI != 0) {
        res.append(str.mid(lastI));
        return res;
    }
    return str;
}

QString qba(const QString &bytes)
{
    QString str = QString::fromLatin1("QByteArray::fromRawData(\"");
    auto esc = cEscape(bytes);
    str += esc + QLatin1String("\", ") + QString::number(esc.length()) + QLatin1String(")");
    return str;
}

QString strLit(const QString &str)
{
    if (str.isNull())
        return QStringLiteral("QString()");
    return QStringLiteral("QStringLiteral(\"%1\")").arg(cEscape(str));
}

const char *headerStart =
        "#include <QScxmlLib/scxmlstatetable.h>\n"
        "\n";

using namespace DocumentModel;

class DataModelWriter
{
public:
    virtual ~DataModelWriter()
    {}
};

class NullDataModelWriter: public DataModelWriter
{
public:
};

class EcmaScriptDataModelWriter: public DataModelWriter
{
public:
};

class DumperVisitor: public DocumentModel::NodeVisitor
{
    Q_DISABLE_COPY(DumperVisitor)

public:
    DumperVisitor(MainClass &clazz, const QString &mainClassName, const CppDumpOptions &options, DataModelWriter *dataModelWriter)
        : clazz(clazz)
        , m_mainClassName(mainClassName)
        , m_options(options)
        , m_dataModelWriter(dataModelWriter)
    {}

    void process(ScxmlDocument *doc)
    {
        doc->root->accept(this);

        addEvents();
    }

    ~DumperVisitor()
    {
        Q_ASSERT(m_parents.isEmpty());
    }

protected:
    using NodeVisitor::visit;

    bool visit(Scxml *node) Q_DECL_OVERRIDE
    {
        // init:
        if (!node->name.isEmpty()) {
            clazz.init.impl << QStringLiteral("table._name = QStringLiteral(\"") + cEscape(node->name) + QStringLiteral("\");");
            if (m_options.nameQObjects)
                clazz.init.impl << QStringLiteral("table.setObjectName(QStringLiteral(\"") + cEscape(node->name) + QStringLiteral("\"));");
        }
        QString dmName;
        switch (node->dataModel) {
        case Scxml::NullDataModel:
            dmName = QStringLiteral("Null");
            clazz.implIncludes << QStringLiteral("QScxmlLib/nulldatamodel.h");
            break;
        case Scxml::JSDataModel:
            dmName = QStringLiteral("EcmaScript");
            clazz.implIncludes << QStringLiteral("QScxmlLib/ecmascriptdatamodel.h");
            break;
        default:
            Q_UNREACHABLE();
        }
        clazz.init.impl << QStringLiteral("table.setDataModel(new Scxml::") + dmName + QStringLiteral("DataModel(&table));");

        QString binding;
        switch (node->binding) {
        case DocumentModel::Scxml::EarlyBinding:
            binding = QStringLiteral("Early");
            break;
        case DocumentModel::Scxml::LateBinding:
            binding = QStringLiteral("Late");
            break;
        default:
            Q_UNREACHABLE();
        }
        clazz.init.impl << QStringLiteral("table.setDataBinding(Scxml::StateTable::%1Binding);").arg(binding);

        foreach (AbstractState *s, node->initialStates) {
            clazz.init.impl << QStringLiteral("table.setInitialState(&state_") + mangledName(s) + QStringLiteral(");");
        }

        // visit the kids:
        m_parents.append(node);
        visit(node->children);
        visit(node->dataElements);

        int iCnt = node->initialSetup.size();
        if (node->script)
            ++iCnt;
        if (iCnt > 0) {
            clazz.init.impl << QStringLiteral("{ Scxml::ExecutableContent::InstructionSequence seq;");
            clazz.init.impl << QStringLiteral("  seq.statements.reserve(%1);").arg(iCnt);
            m_currentInstructionSequence = QStringLiteral("  seq");
            if (node->script)
                node->script->accept(this);
            visit(&node->initialSetup);
            clazz.init.impl << QStringLiteral("  table.setInitialSetup(seq);");
            clazz.init.impl << QStringLiteral("}");
            m_currentInstructionSequence.clear();
        }
        m_parents.removeLast();
        return false;
    }

    bool visit(State *node) Q_DECL_OVERRIDE
    {
        auto stateName = QStringLiteral("state_") + mangledName(node);
        // Declaration:
        if (node->type == State::Final) {
            clazz.classFields << QStringLiteral("Scxml::ScxmlFinalState ") + stateName + QLatin1Char(';');
        } else {
            clazz.classFields << QStringLiteral("Scxml::ScxmlState ") + stateName + QLatin1Char(';');
        }

        // Initializer:
        clazz.constructor.initializer << generateInitializer(node);

        // init:
        if (!node->id.isEmpty() && m_options.nameQObjects) {
            clazz.init.impl << stateName + QStringLiteral(".setObjectName(QStringLiteral(\"") + cEscape(node->id) + QStringLiteral("\"));");
        }
        if (node->initialState) {
            clazz.init.impl << stateName + QStringLiteral(".setInitialState(&state_") + mangledName(node->initialState) + QStringLiteral(");");
        }
        if (node->type == State::Parallel) {
            clazz.init.impl << stateName + QStringLiteral(".setChildMode(QState::ParallelStates);");
        }

        // visit the kids:
        m_parents.append(node);
        visit(node->dataElements);
        visit(node->children);
        generate(stateName + QStringLiteral(".onEntryInstructions"), node->onEntry);
        generate(stateName + QStringLiteral(".onExitInstructions"), node->onExit);
        if (node->doneData)
            node->doneData->accept(this);
        m_parents.removeLast();
        return false;
    }

    bool visit(Transition *node) Q_DECL_OVERRIDE
    {
        const QString tName = transitionName(node);
        m_knownEvents.unite(node->events.toSet());

        // Declaration:
        clazz.classFields << QStringLiteral("Scxml::ScxmlTransition ") + tName + QLatin1Char(';');

        // Initializer:
        QString parentName;
        auto parent = m_parents.last();
        if (HistoryState *historyState = parent->asHistoryState()) {
            parentName = QStringLiteral("state_") + mangledName(historyState) + QStringLiteral("_defaultConfiguration");
        } else {
            parentName = parentStateMemberName();
        }
        Q_ASSERT(!parentName.isEmpty());
        QString initializer = tName + QStringLiteral("(&") + parentName + QStringLiteral(", QList<QByteArray>()");
        foreach (const QString &event, node->events) {
            initializer += QStringLiteral(" << ") + qba(event);
        }
        initializer += QLatin1Char(')');
        clazz.constructor.initializer << initializer;

        // init:
        if (node->condition) {
            QString condExpr = *node->condition.data();
            QString ctxt = createContext(QStringLiteral("transition"), QStringLiteral("cond"), condExpr);
            QString cond = createEvaluator(QStringLiteral("ToBool"), 1, { condExpr, ctxt });
            clazz.init.impl << tName + QStringLiteral(".conditionalExp = %1;").arg(cond);
        }
        clazz.init.impl << parentName + QStringLiteral(".addTransition(&") + tName + QStringLiteral(");");
        if (node->type == Transition::Internal) {
            clazz.init.impl << tName + QStringLiteral(".setTransitionType(QAbstractTransition::InternalTransition);");
        }
        QString targets = tName + QStringLiteral(".setTargetStates(QList<QAbstractState *>()");
        foreach (AbstractState *target, node->targetStates) {
            targets += QStringLiteral(" << &state_") + mangledName(target);
        }
        clazz.init.impl << targets + QStringLiteral(");");

        // visit the kids:
        m_parents.append(node);
        m_currentTransitionName = tName;
        m_currentInstructionSequence = tName + QStringLiteral(".instructionsOnTransition");
        if (!node->instructionsOnTransition.isEmpty())
            clazz.init.impl << m_currentInstructionSequence + QStringLiteral(".statements.reserve(%1);").arg(node->instructionsOnTransition.size());
        return true;
    }

    void endVisit(Transition *) Q_DECL_OVERRIDE
    {
        m_currentInstructionSequence.clear();
        m_parents.removeLast();
        m_currentTransitionName.clear();
    }

    bool visit(DocumentModel::HistoryState *node) Q_DECL_OVERRIDE
    {
        // Includes:
        clazz.implIncludes << "QHistoryState";

        auto stateName = QStringLiteral("state_") + mangledName(node);
        // Declaration:
        clazz.classFields << QStringLiteral("QHistoryState ") + stateName + QLatin1Char(';');

        // Initializer:
        clazz.constructor.initializer << generateInitializer(node);

        // init:
        if (!node->id.isEmpty() && m_options.nameQObjects) {
            clazz.init.impl << stateName + QStringLiteral(".setObjectName(QStringLiteral(\"") + cEscape(node->id) + QStringLiteral("\"));");
        }
        QString depth;
        switch (node->type) {
        case DocumentModel::HistoryState::Shallow:
            depth = QStringLiteral("Shallow");
            break;
        case DocumentModel::HistoryState::Deep:
            depth = QStringLiteral("Deep");
            break;
        default:
            Q_UNREACHABLE();
        }
        clazz.init.impl << stateName + QStringLiteral(".setHistoryType(QHistoryState::") + depth + QStringLiteral("History);");

        // visit the kids:
        if (Transition *t = node->defaultConfiguration()) {
            // Declaration:
            clazz.classFields << QStringLiteral("QState ") + stateName + QStringLiteral("_defaultConfiguration;");

            // Initializer:
            QString init = stateName + QStringLiteral("_defaultConfiguration(&state_");
            if (State *parentState = node->parent->asState()) {
                init += mangledName(parentState);
            }
            clazz.constructor.initializer << init + QLatin1Char(')');

            // init:
            clazz.init.impl << stateName + QStringLiteral(".setDefaultState(&")
                               + stateName + QStringLiteral("_defaultConfiguration);");

            //  visit the kid:
            m_parents.append(node);
            t->accept(this);
            m_parents.removeLast();
        }
        return false;
    }

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE
    {
        QString eventexpr = createEvaluator(QStringLiteral("ToString"), 1, { node->eventexpr, createContext(QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr) });
        QString typeexpr = createEvaluator(QStringLiteral("ToString"), 1, { node->typeexpr, createContext(QStringLiteral("send"), QStringLiteral("typeexpr"), node->typeexpr) });
        QString targetexpr = createEvaluator(QStringLiteral("ToString"), 1, { node->targetexpr, createContext(QStringLiteral("send"), QStringLiteral("targetexpr"), node->targetexpr) });
        QString delayexpr = createEvaluator(QStringLiteral("ToString"), 1, { node->delayexpr, createContext(QStringLiteral("send"), QStringLiteral("delayexpr"), node->delayexpr) });
        generateParams(node->params);
        QString namelist = QStringLiteral("QStringList()");
        foreach (const QString &name, node->namelist)
            namelist += QStringLiteral(" << ") + strLit(name);
        addInstruction(QStringLiteral("Send"), {
                           strLit(createContext(QStringLiteral("send"))),
                           qba(node->event),
                           eventexpr,
                           strLit(node->type),
                           typeexpr,
                           strLit(node->target),
                           targetexpr,
                           strLit(node->id),
                           strLit(node->idLocation),
                           strLit(node->delay),
                           delayexpr,
                           namelist,
                           QStringLiteral("params"),
                           strLit(node->content)
                       });
        clazz.init.impl << QStringLiteral("}");
        return false;
    }

    void visit(Raise *node) Q_DECL_OVERRIDE
    {
        if (!m_currentInstructionSequence.isEmpty()) // TODO: remove this check.
        addInstruction(QStringLiteral("Raise"), { qba(node->event) });
    }

    void visit(Log *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("log"), QStringLiteral("expr"), node->expr);
        QString expr = createEvaluator(QStringLiteral("ToString"), 1, { node->expr, context });
        addInstruction(QStringLiteral("Log"), { strLit(node->label), expr });
    }

    void visit(DataElement *data) Q_DECL_OVERRIDE
    {
        QString code = QStringLiteral("table.dataModel()->addData(Scxml::DataModel::Data(%1, %2, %3, %4));");
        QString parent = QStringLiteral("&") + parentStateMemberName();
        clazz.init.impl << code.arg(strLit(data->id), strLit(data->src), strLit(data->expr), parent);
    }

    void visit(Script *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("script"), QStringLiteral("source"), node->content);
        auto func = createEvaluator(QStringLiteral("Script"), 1, { node->content, context });
        addInstruction(QStringLiteral("JavaScript"), { func });
    }

    void visit(Assign *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("assign"), QStringLiteral("expr"), node->expr);
        QString expr = createEvaluator(QStringLiteral("Assignment"), 2, { node->location, node->expr, context });
        addInstruction(QStringLiteral("AssignExpression"), { expr });
    }

    bool visit(If *node) Q_DECL_OVERRIDE
    {
        clazz.init.impl << QStringLiteral("{ QVector<Scxml::DataModel::ToBoolEvaluator> conditions;");
        clazz.init.impl << QStringLiteral("  conditions.reserve(%1);").arg(node->conditions.size());
        QString tag = QStringLiteral("if");
        for (int i = 0, ei = node->conditions.size(); i != ei; ++i) {
            QString condExpr = node->conditions.at(i);
            QString ctxt = createContext(tag, QStringLiteral("cond"), condExpr);
            QString cond = createEvaluator(QStringLiteral("ToBool"), 1, { condExpr, ctxt });
            clazz.init.impl << QStringLiteral("  conditions.append(%1);").arg(cond);
            if (i == 0) {
                tag = QStringLiteral("elif");
            }
        }
        clazz.init.impl << QStringLiteral("  Scxml::ExecutableContent::InstructionSequences *blocks = new Scxml::ExecutableContent::InstructionSequences;");
        generate(QStringLiteral("(*blocks)"), node->blocks);
        addInstruction(QStringLiteral("If"), { QStringLiteral("conditions"), QStringLiteral("blocks") });
        clazz.init.impl << QStringLiteral("}");
        return false;
    }

    bool visit(Foreach *node) Q_DECL_OVERRIDE
    {
        QString ctxt = createContext(QStringLiteral("foreach"));
        QString it = createEvaluator(QStringLiteral("Foreach"), 0, { node->array, node->item, node->index, ctxt });
        QString previous = m_currentInstructionSequence;
        ExecutableContent::InstructionSequence seq;
        ++m_sequenceDepth;
        clazz.init.impl << QStringLiteral("{ Scxml::ExecutableContent::InstructionSequence seq%1;").arg(m_sequenceDepth);
        clazz.init.impl << QStringLiteral("  seq%1.statements.reserve(%2);").arg(m_sequenceDepth).arg(node->block.size());
        m_currentInstructionSequence = QStringLiteral("  seq%1").arg(m_sequenceDepth);
        visit(&node->block);
        m_currentInstructionSequence = previous;
        addInstruction(QStringLiteral("Foreach"), { it, QStringLiteral("seq%1").arg(m_sequenceDepth) });
        clazz.init.impl << QStringLiteral("}");
        --m_sequenceDepth;
        return false;

    }

    void visit(Cancel *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
        QString sendidexpr = createEvaluator(QStringLiteral("ToString"), 1, { node->sendidexpr, context });
        addInstruction(QStringLiteral("Cancel"), { qba(node->sendid), sendidexpr });
    }

    bool visit(Invoke *) Q_DECL_OVERRIDE
    {
        Q_UNIMPLEMENTED();
        return false;
    }

    bool visit(DocumentModel::DoneData *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("donedata"), QStringLiteral("expr"), node->expr);
        QString expr = createEvaluator(QStringLiteral("ToString"), 1, { node->expr, context });
        generateParams(node->params);
        clazz.init.impl << QStringLiteral("  %1.setDoneData(Scxml::ExecutableContent::DoneData(%2, %3, %4));").arg(parentStateMemberName(), strLit(node->contents), expr, QStringLiteral("params"));
        clazz.init.impl << QStringLiteral("}");
        return false;
    }

private:
    void generateParams(const QVector<Param *> &params)
    {
        clazz.init.impl << QStringLiteral("{ QVector<Scxml::ExecutableContent::Param> params;");
        if (!params.isEmpty())
            clazz.init.impl << QStringLiteral("  params.reserve(%1);").arg(params.size());
        foreach (DocumentModel::Param *p, params) {
            QString context = createContext(QStringLiteral("param"), QStringLiteral("expr"), p->expr);
            QString eval = createEvaluator(QStringLiteral("ToVariant"), 1, { p->expr, context });
            clazz.init.impl << QStringLiteral("  params.append(Scxml::ExecutableContent::Param(%1, %2, %3));").arg(strLit(p->name), eval, strLit(p->location));
        }
    }

    void addInstruction(const QString &instr, const QStringList &args)
    {
        static QString callTemplate = QStringLiteral("%1.statements.append(Scxml::ExecutableContent::Instruction::Ptr(new Scxml::ExecutableContent::%2(%3)));");
        clazz.init.impl << callTemplate.arg(m_currentInstructionSequence, instr, args.join(QStringLiteral(", ")));
    }

    QString createEvaluator(const QString &kind, ushort argToCheckForEmpty, const QStringList &args)
    {
        // TODO: move to datamodel
        if (argToCheckForEmpty > 0 && args.at(argToCheckForEmpty - 1).isEmpty())
            return QStringLiteral("nullptr");

        static QString callTemplate = QStringLiteral("table.dataModel()->create%1Evaluator(%2)");
        QStringList strArgs;
        foreach (const QString &arg, args)
            strArgs.append(strLit(arg));
        return callTemplate.arg(kind, strArgs.join(QStringLiteral(", ")));
    }

    QString mangledName(AbstractState *state)
    {
        Q_ASSERT(state);

        QString name = m_mangledNames.value(state);
        if (!name.isEmpty())
            return name;

        QString id = state->id;
        if (State *s = state->asState()) {
            if (s->type == State::Initial) {
                id = s->parent->asState()->id + QStringLiteral("_initial");
            }
        }

        name = CppDumper::mangleId(id);
        m_mangledNames.insert(state, name);
        return name;
    }

    QString transitionName(Transition *t)
    {
        int idx = 0;
        QString parentName;
        auto parent = m_parents.last();
        if (State *parentState = parent->asState()) {
            parentName = mangledName(parentState);
            idx = childIndex(t, parentState->children);
        } else if (HistoryState *historyState = parent->asHistoryState()) {
            parentName = mangledName(historyState);
        } else if (Scxml *scxml = parent->asScxml()) {
            parentName = QStringLiteral("table");
            idx = childIndex(t, scxml->children);
        } else {
            Q_UNREACHABLE();
        }
        return QStringLiteral("transition_%1_%2").arg(parentName, QString::number(idx));
    }

    static int childIndex(StateOrTransition *child, const QVector<StateOrTransition *> &children) {
        int idx = 0;
        foreach (StateOrTransition *sot, children) {
            if (sot == child)
                break;
            else
                ++idx;
        }
        return idx;
    }

    QString generateInitializer(AbstractState *node)
    {
        auto stateName = QStringLiteral("state_") + mangledName(node);
        QString init = stateName + QStringLiteral("(&");
        if (State *parentState = node->parent->asState()) {
            init += QStringLiteral("state_") + mangledName(parentState);
        } else {
            init += QStringLiteral("table");
        }
        init += QLatin1Char(')');
        return init;
    }

    void addEvents()
    {
        QStringList knownEventsList = m_knownEvents.toList();
        std::sort(knownEventsList.begin(), knownEventsList.end());
        foreach (QString event, knownEventsList) {
            if (event.startsWith(QStringLiteral("done.")) || event.startsWith(QStringLiteral("qsignal."))
                    || event.startsWith(QStringLiteral("qevent."))) {
                continue;
            }
            if (event.contains(QLatin1Char('*')))
                continue;

            clazz.publicSlotDeclarations << QStringLiteral("void event_") + CppDumper::mangleId(event) + QStringLiteral("();");
            clazz.publicSlotDefinitions << QStringLiteral("void ") + m_mainClassName
                                           + QStringLiteral("::event_")
                                           + CppDumper::mangleId(event)
                                           + QStringLiteral("()\n{ submitEvent(") + qba(event)
                                           + QStringLiteral("); }");
        }
    }

    void generate(const QString &outSequences, const InstructionSequences &inSequences)
    {
        QString previous = m_currentInstructionSequence;
        if (!inSequences.isEmpty())
            clazz.init.impl << outSequences + QStringLiteral(".sequences.reserve(%1);").arg(inSequences.size());
        foreach (DocumentModel::InstructionSequence *sequence, inSequences) {
            ++m_sequenceDepth;
            clazz.init.impl << QStringLiteral("{ Scxml::ExecutableContent::InstructionSequence &seq%1 = *%2.newInstructions();")
                               .arg(QString::number(m_sequenceDepth), outSequences);
            if (sequence->isEmpty()) {
                clazz.init.impl << QStringLiteral("  Q_UNUSED(seq%1);").arg(m_sequenceDepth);
            } else {
                clazz.init.impl << QStringLiteral("  seq%1.statements.reserve(%2);").arg(m_sequenceDepth).arg(sequence->size());
            }
            m_currentInstructionSequence = QStringLiteral("  seq%1").arg(m_sequenceDepth);
            visit(sequence);
            clazz.init.impl << QStringLiteral("}");
            --m_sequenceDepth;
        }
        m_currentInstructionSequence = previous;
    }

    QString createContext(const QString &instrName) const
    {
        if (!m_currentTransitionName.isEmpty()) {
            QString state = parentStateName();
            return QStringLiteral("<%1> instruction in transition of state '%2'").arg(instrName, state);
        } else {
            return QStringLiteral("<%1> instruction in state '%2'").arg(instrName, parentStateName());
        }
    }

    QString createContext(const QString &instrName, const QString &attrName, const QString &attrValue) const
    {
        QString location = createContext(instrName);
        return QStringLiteral("%1 with %2=\"%3\"").arg(location, attrName, attrValue);
    }

    QString parentStateName() const
    {
        for (int i = m_parents.size() - 1; i >= 0; --i) {
            Node *node = m_parents.at(i);
            if (State *s = node->asState())
                return s->id;
            else if (HistoryState *h = node->asHistoryState())
                return h->id;
            else if (Scxml *l = node->asScxml())
                return l->name;
        }

        return QString();
    }

    QString parentStateMemberName()
    {
        Node *parent = m_parents.last();
        if (State *s = parent->asState())
            return QStringLiteral("state_") + mangledName(s);
        else if (HistoryState *h = parent->asHistoryState())
            return QStringLiteral("state_") + mangledName(h);
        else if (parent->asScxml())
            return QStringLiteral("table");
        else
            Q_UNIMPLEMENTED();
        return QString();
    }

private:
    MainClass &clazz;
    const QString &m_mainClassName;
    const CppDumpOptions &m_options;
    DataModelWriter *m_dataModelWriter;
    QHash<DocumentModel::AbstractState *, QString> m_mangledNames;
    QVector<Node *> m_parents;
    QString m_currentInstructionSequence;
    QSet<QString> m_knownEvents;
    QString m_currentTransitionName;
    int m_sequenceDepth = 0;
};
} // anonymous namespace

void CppDumper::dump(DocumentModel::ScxmlDocument *doc)
{
    m_doc = doc;
    mainClassName = options.classname;
    if (mainClassName.isEmpty()) {
        mainClassName = mangleId(doc->root->name);
        if (!mainClassName.isEmpty())
            mainClassName.append(QLatin1Char('_'));
        mainClassName.append(l("StateMachine"));
    }

    MainClass clazz;
    QScopedPointer<DataModelWriter> dataModelWriter;
    switch (doc->root->dataModel) {
    case Scxml::NullDataModel:
        dataModelWriter.reset(new NullDataModelWriter);
        break;
    case Scxml::JSDataModel:
        dataModelWriter.reset(new EcmaScriptDataModelWriter);
        break;
    default:
        Q_UNREACHABLE();
    }

    DumperVisitor(clazz, mainClassName, options, dataModelWriter.data()).process(doc);

    // Generate the .h file:
    const QString headerGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
    h << QStringLiteral("#ifndef ") << headerGuard << endl
      << QStringLiteral("#define ") << headerGuard << endl
      << endl;
    h << l(headerStart);
    if (!options.namespaceName.isEmpty())
        h << l("namespace ") << options.namespaceName << l(" {") << endl << endl;
    h << l("class ") << mainClassName << l(" : public Scxml::StateTable\n{") << endl;
    h << QLatin1String("    Q_OBJECT\n\n");
    h << QLatin1String("public:\n");
    h << l("    ") << mainClassName << l("(QObject *parent = 0);") << endl;
    h << l("    ~") << mainClassName << "();" << endl;
    h << endl
      << l("    bool init() Q_DECL_OVERRIDE;") << endl;

    if (!clazz.publicSlotDeclarations.isEmpty()) {
        h << "\npublic slots:\n";
        clazz.publicSlotDeclarations.write(h, QStringLiteral("    "), QStringLiteral("\n"));
    }

    h << endl
      << l("private:") << endl
      << l("    struct Data;") << endl
      << l("    struct Data *data;") << endl
      << l("};") << endl;

    if (!options.namespaceName.isEmpty())
        h << endl << l("} // namespace ") << options.namespaceName << endl;
    h << endl
      << QStringLiteral("#endif // ") << headerGuard << endl;

    // Generate the .cpp file:
    cpp << l("#include \"") << headerName << l("\"") << endl
        << endl;
    if (!clazz.implIncludes.isEmpty()) {
        clazz.implIncludes.write(cpp, QStringLiteral("#include <"), QStringLiteral(">\n"));
        cpp << endl;
    }
    if (!options.namespaceName.isEmpty())
        cpp << l("namespace ") << options.namespaceName << l(" {") << endl;

    cpp << l("struct ") << mainClassName << l("::Data {") << endl;

    cpp << l("    Data(Scxml::StateTable &table)\n        : table(table)") << endl;
    clazz.constructor.initializer.write(cpp, QStringLiteral("        , "), QStringLiteral("\n"));
    cpp << l("    {}") << endl;

    cpp << endl;
    cpp << l("    void init() {\n");
    clazz.init.impl.write(cpp, QStringLiteral("        "), QStringLiteral("\n"));
    cpp << l("    }") << endl;


    cpp << endl
        << l("    Scxml::StateTable &table;") << endl;
    clazz.classFields.write(cpp, QStringLiteral("    "), QStringLiteral("\n"));

    cpp << l("};") << endl
        << endl;
    cpp << mainClassName << l("::") << mainClassName << l("(QObject *parent)") << endl
        << l("    : Scxml::StateTable(parent)") << endl
        << l("    , data(new Data(*this))") << endl
        << l("{}") << endl
        << endl;
    cpp << mainClassName << l("::~") << mainClassName << l("()") << endl
        << l("{ delete data; }") << endl
        << endl;
    cpp << l("bool ") << mainClassName << l("::init()") << endl
        << l("{ data->init(); return StateTable::init(); }") << endl;
    cpp << endl;
    clazz.publicSlotDefinitions.write(cpp, QStringLiteral("\n"), QStringLiteral("\n"));
    cpp << endl;

    if (!options.namespaceName.isEmpty())
        cpp << l("} // namespace ") << options.namespaceName << endl;
}

QString CppDumper::mangleId(const QString &id)
{
    QString mangled(id);
    mangled = mangled.replace(QLatin1Char('_'), QLatin1String("__"));
    mangled = mangled.replace(QLatin1Char(':'), QLatin1String("_colon_"));
    mangled = mangled.replace(QLatin1Char('-'), QLatin1String("_dash_"));
    mangled = mangled.replace(QLatin1Char('@'), QLatin1String("_at_"));
    mangled = mangled.replace(QLatin1Char('.'), QLatin1String("_dot_"));
    return mangled;
}

} // namespace Scxml
