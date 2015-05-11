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

//StringListDumper &endl(StringListDumper &s)
//{
//    s << QLatin1String("\n");
//    return s;
//}

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

//QByteArray cEscape(const QByteArray &str)
//{ // should handle even trigraphs?
//    QByteArray res;
//    int lastI = 0;
//    for (int i = 0; i < str.length(); ++i) {
//        unsigned char c = str.at(i);
//        if (c < ' ' || c == '\\' || c == '\"') {
//            res.append(str.mid(lastI, i - lastI));
//            lastI = i + 1;
//            if (c == '\\') {
//                res.append("\\\\");
//            } else if (c == '\"') {
//                res.append("\"");
//            } else {
//                char buf[4];
//                buf[0] = '\\';
//                buf[3] = '0' + (c & 0x7);
//                c >>= 3;
//                buf[2] = '0' + (c & 0x7);
//                c >>= 3;
//                buf[1] = '0' + (c & 0x7);
//                res.append(&buf[0], 4);
//            }
//        }
//    }
//    if (lastI != 0) {
//        res.append(str.mid(lastI));
//        return res;
//    }
//    return str;
//}

QString cEscape(const QString &str)
{// should handle even trigraphs?
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

//QString qba(const QByteArray &bytes)
//{
//    QString str = QString::fromLatin1("QByteArray::fromRawData(\"");
//    auto esc = cEscape(bytes);
//    str += QString::fromLatin1(esc) + QLatin1String("\", ") + QString::number(esc.length()) + QLatin1String(")");
//    return str;
//}

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
            QString cond = createEvaluator(QStringLiteral("ToBool"), { condExpr, ctxt });
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
                               + stateName + QStringLiteral(");");

            //  visit the kid:
            m_parents.append(node);
            t->accept(this);
            m_parents.removeLast();
        }
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
        QString expr = createEvaluator(QStringLiteral("ToString"), { node->expr, context });
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
        auto func = createEvaluator(QStringLiteral("Script"), { node->content, context });
        addInstruction(QStringLiteral("JavaScript"), { func });
    }

    void visit(Assign *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("assign"), QStringLiteral("expr"), node->expr);
        QString expr = createEvaluator(QStringLiteral("Assignment"), { node->location, node->expr, context });
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
            QString cond = createEvaluator(QStringLiteral("ToBool"), { condExpr, ctxt });
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
        QString it = createEvaluator(QStringLiteral("Foreach"), { node->array, node->item, node->index, ctxt });
        QString previous = m_currentInstructionSequence;
        ExecutableContent::InstructionSequence seq;
        clazz.init.impl << QStringLiteral("{ Scxml::ExecutableContent::InstructionSequence seq;");
        clazz.init.impl << QStringLiteral("  seq.statements.reserve(%1);").arg(node->block.size());
        m_currentInstructionSequence = QStringLiteral("  seq");
        visit(&node->block);
        m_currentInstructionSequence = previous;
        addInstruction(QStringLiteral("Foreach"), { it, QStringLiteral("seq" )});
        clazz.init.impl << QStringLiteral("}");
        return false;

    }

    void visit(Cancel *node) Q_DECL_OVERRIDE
    {
        QString context = createContext(QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
        QString sendidexpr = createEvaluator(QStringLiteral("ToString"), { node->sendidexpr, context });
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
        QString expr = createEvaluator(QStringLiteral("ToString"), { node->expr, context });
        clazz.init.impl << QStringLiteral("{ QVector<Scxml::ExecutableContent::Param> params;");
        clazz.init.impl << QStringLiteral("  params.reserve(%1);").arg(node->params.size());
        foreach (DocumentModel::Param *p, node->params) {
            QString context = createContext(QStringLiteral("param"), QStringLiteral("expr"), p->expr);
            QString eval = createEvaluator(QStringLiteral("ToVariant"), { p->expr, context });
            clazz.init.impl << QStringLiteral("  params.append(Scxml::ExecutableContent::Param(%1, %2, %3));").arg(strLit(p->name), eval, strLit(p->location));
        }
        clazz.init.impl << QStringLiteral("  %1.setDoneData(Scxml::ExecutableContent::DoneData(%2, %3, %4));").arg(parentStateMemberName(), strLit(node->contents), expr, QStringLiteral("params"));
        clazz.init.impl << QStringLiteral("}");
        return false;
    }

private:
    void addInstruction(const QString &instr, const QStringList &args)
    {
        static QString callTemplate = QStringLiteral("%1.statements.append(Scxml::ExecutableContent::Instruction::Ptr(new Scxml::ExecutableContent::%2(%3)));");
        clazz.init.impl << callTemplate.arg(m_currentInstructionSequence, instr, args.join(QStringLiteral(", ")));
    }

    QString createEvaluator(const QString &kind, const QStringList &args)
    {
        // TODO: move to datamodel
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
        if (inSequences.isEmpty())
            return;

        QString previous = m_currentInstructionSequence;
        clazz.init.impl << outSequences + QStringLiteral(".sequences.reserve(%1);").arg(inSequences.size());
        foreach (DocumentModel::InstructionSequence *sequence, inSequences) {
            if (sequence->isEmpty())
                continue;
            clazz.init.impl << QStringLiteral("{ Scxml::ExecutableContent::InstructionSequence &seq = *")
                               + outSequences + QStringLiteral(".newInstructions();");
            clazz.init.impl << QStringLiteral("  seq.statements.reserve(%1);").arg(sequence->size());
            m_currentInstructionSequence = QStringLiteral("  seq");
            visit(sequence);
            clazz.init.impl << QStringLiteral("}");
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
    cpp << endl
        << l("    }") << endl;


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

void CppDumper::dumpExecutableContent()
{
#if 0
    // dump special transitions
    bool inSlots = false;
    loopOnSubStates(table, [this, &inSlots](QState *state) -> bool {
        QString stateName = mangledName(state);
        if (ScxmlState *sState = qobject_cast<ScxmlState *>(state)) {
            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onEntryInstruction, sState->onEntryInstructions) {
                if (!onEntryInstruction->statements.isEmpty()) {
                    if (!inSlots) {
                        cpp << l("public slots:\n");
                        inSlots = true;
                    }
                    cpp << l("        void onEnter_") << stateName
                        << l("() {\n");
                    dumpInstructions(onEntryInstruction);
                    cpp << l("        }\n");
                }
            }
            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onExitInstruction, sState->onExitInstructions) {
                if (!onExitInstruction->statements.isEmpty()) {
                    if (!inSlots) {
                        cpp << l("public slots:\n");
                        inSlots = true;
                    }
                    cpp << l("        void onExit_") << stateName
                        << l("() {\n");
                    dumpInstructions(onExitInstruction);
                    cpp << l("        }\n\n");
                }
            }
        }
        for (int tIndex = 0; tIndex < state->transitions().size(); ++tIndex) {
            QAbstractTransition *t = state->transitions().at(tIndex);
            if (::Scxml::ScxmlTransition *scTransition = qobject_cast<::Scxml::ScxmlTransition *>(t)) {
                Q_UNIMPLEMENTED(); // FIXME
                /*if (scTransition->conditionalExp.isEmpty()) {
                    if (scTransition->instructionsOnTransition.statements.isEmpty())
                        continue;
                    if (!inSlots) {
                        cpp << l("public slots:\n");
                        inSlots = true;
                    }
                    cpp << l("        void on") << transitionName(scTransition, true, tIndex, stateName)
                        << l("() {\n");
                    dumpInstructions(&scTransition->instructionsOnTransition);
                    cpp << l("        }\n\n");
                } else*/ {
                    if (inSlots) { // avoid ?
                        cpp << l("public:\n");
                        inSlots = false;
                    }
//                    cpp << l("    class ") << transitionName(scTransition, true, tIndex, stateName) << " : Scxml::ScxmlBaseTransition {\n";
                    Q_UNIMPLEMENTED(); // FIXME
//                    if (!scTransition->conditionalExp.isEmpty()) { // we could cache the function calculating the test
//                        cpp << l("        bool eventTest(QEvent *event) Q_DECL_OVERRIDE {\n");
//                        cpp << l("            if (ScxmlBaseTransition::testEvent(e)\n");
//                        cpp << l("                    && table()->evalBool(\"")
//                            << cEscape(scTransition->conditionalExp) << l("\")\n");
//                        cpp << l("                return true;\n");
//                        cpp << l("            return false;\n");
//                        cpp << l("        }\n");
//                    }
//                    if (!scTransition->instructionsOnTransition.statements.isEmpty()) {
//                        cpp << l("    protected:\n");
//                        cpp << l("        void onTransition(QEvent *event) Q_DECL_OVERRIDE {\n");
//                        dumpInstructions(&scTransition->instructionsOnTransition);
//                        cpp << l("        }\n");
//                    }
//                    cpp << l("    };\n\n");
                }
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    if (inSlots) {
        cpp << l("public:\n");
    }
#endif
}

void CppDumper::dumpInit()
{
//    StringListDumper sIni; // state initialization
//    StringListDumper tIni; // transition initialization

//    loopOnSubStates(table, [this,&sIni,&tIni](QState *state) -> bool {
//        QByteArray rawStateName = state->objectName().toUtf8();
//        QString stateName = mangleId(rawStateName);
//        sIni << l("        table->addId(") << qba(rawStateName) << l(", &state_") << stateName
//             << l(");\n");
//        if (options.nameQObjects)
//            sIni << l("        state_") << stateName
//                 << l(".setObjectName(QStringLiteral(\"") << cEscape(rawStateName) << l("\"));\n");
//        if (state->childMode() == QState::ParallelStates)
//            sIni << l("        state_") << stateName << l(".setChildMode(QState::ParallelStates);\n");
//        if (ScxmlState *sState = qobject_cast<ScxmlState *>(state)) {
//            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onEntryInstruction, sState->onEntryInstructions) {
//                if (!onEntryInstruction->statements.isEmpty()) {
//                    sIni << l("        QObject::connect(&state_") << stateName
//                         << l(", &QAbstractState::entered, table, &") << mainClassName << l("::onEnter_")
//                         << stateName << l(");\n");
//                }
//            }
//            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onExitInstruction, sState->onExitInstructions) {
//                if (!onExitInstruction->statements.isEmpty()) {
//                    sIni << l("        QObject::connect(&state_") << stateName
//                         << l(", &QAbstractState::exited, table, &") << mainClassName << l("::onExit_")
//                         << stateName << l(");\n");
//                }
//            }
//        }

//        if (state->childMode() == QState::ExclusiveStates && state->initialState()) {
//            tIni << l("\n        state_") << stateName << l(".setInitialState(&state_")
//                 << mangledName(state->initialState()) << l(");\n");
//        }
//        tIni << "\n";
//        for (int tIndex = 0; tIndex < state->transitions().size(); ++tIndex) {
//            QAbstractTransition *t = state->transitions().at(tIndex);
//            if (::Scxml::ScxmlTransition *scTransition = qobject_cast<::Scxml::ScxmlTransition *>(t)) {
//                QString scName = transitionName(scTransition, false, tIndex, stateName);
//                Q_UNIMPLEMENTED(); // FIXME
////                if (scTransition->conditionalExp.isEmpty()) {
////                    if (!scTransition->instructionsOnTransition.statements.isEmpty()) {
////                        tIni << l("        QObject::connect(&") << scName
////                          << l(", &QAbstractTransition::triggered, table, &")
////                          << mainClassName << l("::onTransition_") << tIndex << l("_") << stateName
////                          << l(");\n");
////                    }
////                }
//                QList<QByteArray> targetStates = scTransition->targetIds();
//                if (targetStates.size() == 1) {
//                    tIni << l("        ") << scName << l(".setTargetState(&state_")
//                      << mangleId(targetStates.first()) << l(");\n");
//                } else if (targetStates.size() > 1) {
//                    tIni << l("        ") << scName << l(".setTargetStates(QList<QAbstractState *>() ");
//                    foreach (const QByteArray &tState, targetStates)
//                        tIni << l("\n            << &state_") << mangleId(tState);
//                    tIni << l(");\n");
//                }
//                tIni << l("        ") << scName << l(".init();\n");
//            }
//        }

//        return true;
//    }, nullptr, [this,&sIni](QAbstractState *state) -> void {
//        QByteArray rawStateName = state->objectName().toUtf8();
//        QString stateName = mangleId(rawStateName);
//        sIni << l("        table->addId(") << qba(rawStateName) << l(", &state_") << stateName
//             << l(");\n");
//        if (options.nameQObjects)
//            sIni << l("        state_") << stateName
//                 << l(".setObjectName(QStringLiteral(\"") << cEscape(rawStateName) << l("\"));\n");
//        if (ScxmlFinalState *sState = qobject_cast<ScxmlFinalState *>(state)) {
//            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onEntryInstruction, sState->onEntryInstructions) {
//                if (!onEntryInstruction->statements.isEmpty()) {
//                    sIni << l("        QObject::connect(&state_") << stateName
//                         << l(", &QAbstractState::entered, table, &") << mainClassName << l("::onEnter_")
//                         << stateName << l(");\n");
//                }
//            }
//            foreach (const ::Scxml::ExecutableContent::InstructionSequence *onExitInstruction, sState->onExitInstructions) {
//                if (!onExitInstruction->statements.isEmpty()) {
//                    sIni << l("        QObject::connect(&state_") << stateName
//                         << l(", &QAbstractState::exited, table, &") << mainClassName << l("::onExit_")
//                         << stateName << l(");\n");
//                }
//            }
//        }
//    });

//    cpp << l("    bool init() {\n");
//    sIni.write(cpp, QStringLiteral(""), QStringLiteral(""));

//    if (table->initialState()) {
//        cpp << l("\n        table->setInitialState(&state_")
//            << mangledName(table->initialState()) << l(");") << endl;
//    }

//    tIni.write(cpp, QStringLiteral(""), QStringLiteral(""));
//    cpp << endl;
//    cpp << l("        return true;\n");
//    cpp << l("    }\n");
}

QString CppDumper::transitionName(QAbstractTransition *transition, bool upcase, int tIndex,
                                  const QString &stateName)
{
    Q_ASSERT(transition->sourceState());
    if (tIndex < 0)
        tIndex = transition->sourceState()->transitions().indexOf(transition);
    Q_ASSERT(tIndex >= 0);
    QString stateNameStr;
    if (stateName.isEmpty())
        stateNameStr = mangledName(transition->sourceState());
    else
        stateNameStr = stateName;
    QString name = QStringLiteral("transition_%1_%2")
            .arg(stateNameStr, QString::number(tIndex));
    if (upcase)
        name[0] = QLatin1Char('T');
    return name;
}

QString CppDumper::mangledName(QAbstractState *state)
{
    QString mangledName = mangledStateNames.value(state);
    if (!mangledName.isEmpty())
        return mangledName;

    mangledName = mangleId(state->objectName());
    mangledStateNames.insert(state, mangledName);
    return mangledName;
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
