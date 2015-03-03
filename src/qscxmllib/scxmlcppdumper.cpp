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

#include "scxmlcppdumper.h"
#include <algorithm>

namespace Scxml {

namespace {
using namespace ExecutableContent;
class DumpCppInstructionVisitor : public InstructionVisitor {
    QLatin1String l(const char *str) { return QLatin1String(str); }
    QTextStream &s;
    int indentSize;
    bool didDumpTable;
public:
    DumpCppInstructionVisitor(QTextStream &dumper, int indentSize = 0)
        : s(dumper)
        , indentSize(indentSize)
        , didDumpTable(false) { }
    QTextStream & indented() {
        for (int i = 0; i < indentSize; ++i)
            s << QLatin1Char(' ');
        return s;
    }
protected:
    void maybeTable() {
        if (didDumpTable)
            return;
        didDumpTable = true;
        indented() << "StateTable *sTable = table();\n";
    }

    void visitRaise(Raise *r) Q_DECL_OVERRIDE {
        maybeTable();
        indented() << l("sTable->submitEvent(QByteArray(\"")
                   << CppDumper::cEscape(r->event) << l("\"));\n");
    }

    void visitSend(Send *send) Q_DECL_OVERRIDE {
        maybeTable();
        if (!send->delayexpr.isEmpty() || !send->delay.isEmpty()) {
            if (!send->delayexpr.isEmpty()) {
                indented() << l("int delay = sTable->evalValueInt(QStringLiteral(\"")
                  << CppDumper::cEscape(send->delayexpr)
                  << l("\"), []() -> QString { return QStringLiteral(\"")
                  << CppDumper::cEscape(send->instructionLocation())
                  << l("\"); }, 0);\n");
            } else {
                bool ok;
                send->delay.toInt(&ok);
                if (ok) {
                    indented() << l("int delay = ") << send->delay << l(";\n");
                } else {
                    indented() << l("int delay = 0;");
                    qCWarning(scxmlLog) << "could not convert delay '" << send->delay
                                        << "' to int, using 0 instead in "
                                        << send->instructionLocation();
                }
            }
            indented() << l("sTable->submitDelayedEvent(delay, ");
        } else {
            indented() << l("sTable->submitEvent(");
        }
        if (!send->eventexpr.isEmpty()) {
            s << l("sTable->evalStr(QStringLiteral(\"") << CppDumper::cEscape(send->eventexpr) << l("\"),\n");
        } else {
            s << l("QByteArray(\"") << send->event << l("\"),\n");
            if (send->event.isEmpty())
                qCWarning(scxmlLog) << "missing event from send in " << send->instructionLocation();
        }
        /*if (!send->targetexpr.isEmpty()) {
            l("sTable->evalStr(QStringLiteral(\"") << CppDumper::cEscape(send->targetexpr) << l("\"),\n");
            s.writeAttribute("targetexpr", send->targetexpr);
        else if (!send->target.isEmpty())
            s.writeAttribute("target", send->target);
        if (!send->typeexpr.isEmpty())
            s.writeAttribute("typeexpr", send->typeexpr);
        else if (!send->type.isEmpty())
            s.writeAttribute("type", send->type);
        if (!send->idLocation.isEmpty())
            s.writeAttribute("idlocation", send->idLocation);
        else if (!send->id.isEmpty())
            s.writeAttribute("id", send->id);
        if (!send->namelist.isEmpty())
            s.writeAttribute("namelist", send->namelist.join(QLatin1Char(' ')));
        foreach (const Param &p, send->params) {
            s.writeStartElement("param");
            s.writeAttribute("name", p.name);
            if (!p.location.isEmpty())
                s.writeAttribute("location", p.location);
            else if (!p.expr.isEmpty())
                s.writeAttribute("expr", p.expr);
            s.writeEndElement();
        }
        if (send->content) {
            s.writeStartElement("content");
            send->content->dump(s.s);
            s.writeEndElement();
        }*/
        indented() << l("    QVariant());\n");
    }

    void visitLog(Log *log) Q_DECL_OVERRIDE {
        maybeTable();
        indented() << l("sTable->doLog(QStringLiteral(\"") << CppDumper::cEscape(log->label) << l("\",\n");
        indented() << l("    sTable->evalValueStr( QStringLiteral(\"") << CppDumper::cEscape(log->expr) << l("\")));\n");
    }

    void visitJavaScript(JavaScript *script) Q_DECL_OVERRIDE {
        /*s.writeStartElement("script");
        if (!script->src.isEmpty())
            s.writeAttribute("src", script->src);
        else if (!script->source.isEmpty())
            s.s.writeCharacters(script->source); // escape? use CDATA?
        s.writeEndElement();*/
    }

    void visitAssignJson(AssignJson *assign) Q_DECL_OVERRIDE {
        /*s.writeStartElement("assign");
        s.writeAttribute("location", assign->location);
        QJsonValue &v = assign->value;
        switch (v.type()) {
        case QJsonValue::Null:
            s.s.writeCharacters(QLatin1String("null"));
            break;
        case QJsonValue::Bool:
            if (v.toBool())
                s.s.writeCharacters(QLatin1String("true"));
            else
                s.s.writeCharacters(QLatin1String("false"));
            break;
        case QJsonValue::Double:
            s.s.writeCharacters(QString::number(v.toDouble()));
            break;
        case QJsonValue::String: { // ugly, use direct escaping?
            QJsonArray arr;
            QJsonDocument doc(arr);
            QByteArray json = doc.toJson(QJsonDocument::Compact);
            int from = json.indexOf('[');
            int to = json.lastIndexOf(']');
            s.s.writeCharacters(QString::fromUtf8(json.mid(from + 1, to - from - 1)));
            break;
        }
        case QJsonValue::Array: {
            QJsonDocument doc(assign->value.toArray());
            s.s.writeCharacters(QString::fromUtf8(doc.toJson()));
            break;
        }
        case QJsonValue::Object: {
            QJsonDocument doc(assign->value.toObject());
            s.s.writeCharacters(QString::fromUtf8(doc.toJson()));
            break;
        }
        case QJsonValue::Undefined:
            s.s.writeCharacters(QLatin1String("undefined"));
            break;
        }
        s.writeEndElement();*/
    }

    void visitAssignExpression(AssignExpression *assign) Q_DECL_OVERRIDE {
        /*
        s.writeStartElement("assign");
        s.writeAttribute("location", assign->location);
        if (assign->content)
            assign->content->dump(s.s);
        else if (!assign->expression.isEmpty())
            s.writeAttribute("expr", assign->expression);
        s.writeEndElement();*/
    }

    void visitCancel(Cancel *c) Q_DECL_OVERRIDE {
        /*s.writeStartElement("cancel");
        if (!c->sendidexpr.isEmpty())
            s.writeAttribute("sendidexpr", c->sendidexpr);
        else if (!c->sendid.isEmpty())
            s.writeAttribute("sendid", c->sendid);*/
    }

    bool visitInvoke(Invoke *invoke) Q_DECL_OVERRIDE {
        /*s.writeStartElement("invoke");
        if (!invoke->typeexpr.isEmpty())
            s.writeAttribute("typeexpr", invoke->typeexpr);
        else if (!invoke->type.isEmpty())
            s.writeAttribute("type", invoke->type);
        if (!invoke->srcexpr.isEmpty())
            s.writeAttribute("srcexpr", invoke->srcexpr);
        else if (!invoke->src.isEmpty())
            s.writeAttribute("src", invoke->src);
        if (!invoke->idLocation.isEmpty())
            s.writeAttribute("idlocation", invoke->idLocation);
        else if (!invoke->id.isEmpty())
            s.writeAttribute("id", invoke->id);
        if (!invoke->namelist.isEmpty())
            s.writeAttribute("namelist", invoke->namelist.join(QLatin1Char(' ')));
        if (invoke->autoforward)
            s.writeAttribute("autoforward", "true");
        foreach (const Param &p, invoke->params) {
            s.writeStartElement("param");
            s.writeAttribute("name", p.name);
            if (!p.location.isEmpty())
                s.writeAttribute("location", p.location);
            else if (!p.expr.isEmpty())
                s.writeAttribute("expr", p.expr);
            s.writeEndElement();
        }
        if (!invoke->finalize.statements.isEmpty()) {
            s.writeStartElement("finalize");
            accept(&invoke->finalize);
            s.writeEndElement();
        }
        if (invoke->content) {
            s.writeStartElement("content");
            invoke->content->dump(s.s);
            s.writeEndElement();
        }
        s.writeEndElement();*/
        return false;
    }

    void endVisitInvoke(Invoke *) Q_DECL_OVERRIDE { }

    bool visitIf(If *ifI) Q_DECL_OVERRIDE {
        /*s.writeStartElement("if");
        int maxI = ifI->conditions.size();
        if (ifI->blocks.size() < maxI) {
            qCWarning(scxmlLog) << "if instruction with too few blocks " << ifI->blocks.size()
                                << " for " << ifI->conditions.size() << " conditions "
                                << ifI->instructionLocation();
            maxI = ifI->blocks.size();
        }
        s.writeAttribute("cond", ifI->conditions.value(0));
        if (!ifI->blocks.isEmpty())
            accept(&ifI->blocks[0]);
        for (int i = 1; i < maxI; ++i) {
            s.writeStartElement("elseif");
            s.writeAttribute("cond", ifI->conditions.at(i));
            s.writeEndElement();
            accept(&ifI->blocks[i]);
        }
        if (ifI->blocks.size() > maxI) {
            s.writeStartElement("else");
            s.writeEndElement();
            accept(&ifI->blocks[maxI]);
            if (ifI->blocks.size() > maxI + 1) {
                qCWarning(scxmlLog) << "if instruction with too many blocks " << ifI->blocks.size()
                                    << " for " << ifI->conditions.size() << " conditions "
                                    << ifI->instructionLocation();
            }
        }
        s.writeEndElement();*/
        return false;
    }

    void endVisitIf(If *) Q_DECL_OVERRIDE { }
    bool visitForeach(Foreach *foreachI) {
        /*s.writeStartElement("foreach");
        s.writeAttribute("array", foreachI->array);
        s.writeAttribute("item", foreachI->item);
        if (!foreachI->index.isEmpty())
            s.writeAttribute("index", foreachI->index);*/
        return true;
    }

    void endVisitForeach(Foreach *) Q_DECL_OVERRIDE {
        //s.writeEndElement();
    }
    bool visitSequence(InstructionSequence *) Q_DECL_OVERRIDE { return true; }
    void endVisitSequence(InstructionSequence *) Q_DECL_OVERRIDE { }

};

} // anonymous namespace

const char *headerStart =
        "#include <qscxmllib/scxmlstatetable.h>\n"
        "\n";

void CppDumper::dump(StateTable *table)
{
    this->table = table;
   mainClassName = options.basename;
   if (!mainClassName.isEmpty())
       mainClassName.append(QLatin1Char('_'));
   mainClassName.append(table->_name);
   if (!mainClassName.isEmpty())
       mainClassName.append(QLatin1Char('_'));
   mainClassName.append(l("StateMachine"));
    s << l(headerStart);
    if (!options.namespaceName.isEmpty())
        s << l("namespace ") << options.namespaceName << l(" {\n");
    s << l("class ") << mainClassName << l(" : public Scxml::StateTable {\n");
    s << "Q_OBJECT\n";
    dumpDeclareStates();
    dumpDeclareSignalsForEvents();
    dumpExecutableContent();
    s << "public:\n";
    dumpInit();
    s << l("};\n");
    if (!options.namespaceName.isEmpty())
        s << l("} // namespace ") << options.namespaceName << l("\n");
}


void CppDumper::dumpDeclareStates()
{
    loopOnSubStates(table, [this](QState *state) -> bool {
        s << l("Scxml::ScxmlState *state_") << table->objectId(state, false) << l(";\n");
        return true;
    }, Q_NULLPTR, [this](QAbstractState *state) -> void {
        s << b(state->metaObject()->className()) << l(" *state_") << table->objectId(state, false)
          << l(";\n");
    });
}

void CppDumper::dumpDeclareSignalsForEvents()
{
    QSet<QByteArray> knownEvents;
    loopOnSubStates(table, [&knownEvents](QState *state) -> bool {
        foreach (QAbstractTransition *t, state->transitions()) {
            if (ScxmlTransition *scxmlT = qobject_cast<ScxmlTransition *>(t)) {
                foreach (const QByteArray &event, scxmlT->eventSelector)
                    knownEvents.insert(event);
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    QList<QByteArray> knownEventsList = knownEvents.toList();
    std::sort(knownEventsList.begin(), knownEventsList.end());
    bool hasSignals = false;
    foreach (QByteArray event, knownEventsList) {
        if (event.startsWith(b("done.")) || event.startsWith(b("qsignal."))
                || event.startsWith(b("qevent.")))
            continue;
        if (!hasSignals)
            s << l("signals:\n");
        hasSignals = true;
        s << l("void event_") << event.replace('.', '_') << "();\n";
    }
    if (hasSignals)
        s << l("public:\n");
}

void CppDumper::dumpExecutableContent()
{
    // dump special transitions
    bool inSlots = false;
    loopOnSubStates(table, [this, &inSlots](QState *state) -> bool {
        QByteArray stateName = table->objectId(state);
        if (ScxmlState *sState = qobject_cast<ScxmlState *>(state)) {
            if (!sState->onEntryInstruction.statements.isEmpty()) {
                if (!inSlots) {
                    s << l("public slots:\n");
                    inSlots = true;
                }
                s << l("        void onEnter_") << stateName
                  << l("() {\n");
                dumpInstructions(sState->onEntryInstruction);
                s << l("        }\n");
            }
            if (!sState->onExitInstruction.statements.isEmpty()) {
                if (!inSlots) {
                    s << l("public slots:\n");
                    inSlots = true;
                }
                s << l("        void onExit_") << stateName
                  << l("() {\n");
                dumpInstructions(sState->onEntryInstruction);
                s << l("        }\n\n");
            }
        }
        for (int tIndex = 0; tIndex < state->transitions().size(); ++tIndex) {
            QAbstractTransition *t = state->transitions().at(tIndex);
            if (Scxml::ScxmlTransition *scTransition = qobject_cast<Scxml::ScxmlTransition *>(t)) {
                if (scTransition->conditionalExp.isEmpty()) {
                    if (scTransition->instructionsOnTransition.statements.isEmpty())
                        continue;
                    if (!inSlots) {
                        s << l("public slots:\n");
                        inSlots = true;
                    }
                    s << l("        void onTransition_") << tIndex << l("_") << stateName
                      << l("() {\n");
                    dumpInstructions(scTransition->instructionsOnTransition);
                    s << l("        }\n\n");
                } else {
                    if (inSlots) { // avoid ?
                        s << l("public:\n");
                        inSlots = false;
                    }
                    s << l("    class T_") << tIndex << l("_") << stateName << " : Scxml::ScxmlBaseTransition {\n";
                    if (!scTransition->conditionalExp.isEmpty()) {
                        s << l("        bool eventTest(QEvent *event) Q_DECL_OVERRIDE {\n");
                        s << l("            if (ScxmlBaseTransition::testEvent(e)\n");
                        s << l("                    && table()->evalBool(\"")
                          << cEscape(scTransition->conditionalExp) << l("\")\n");
                        s << l("                return true;\n");
                        s << l("            return false;\n");
                        s << l("        }\n");
                    }
                    if (!scTransition->instructionsOnTransition.statements.isEmpty()) {
                        s << l("    protected:\n");
                        s << l("        void onTransition(QEvent *event) Q_DECL_OVERRIDE {\n");
                        dumpInstructions(scTransition->instructionsOnTransition);
                        s << l("        }\n");
                    }
                    s << l("    };\n\n");
                }
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    if (inSlots) {
        s << l("public:\n");
    }
}

void CppDumper::dumpInstructions(ExecutableContent::Instruction &i)
{
    DumpCppInstructionVisitor visitor(s);
    visitor.accept(&i);
}

void CppDumper::dumpInit()
{
    s << l("    bool init() Q_DECL_OVERRIDE {\n");
    loopOnSubStates(table, [this](QState *state) -> bool {
        QString stateName = QString::fromUtf8(table->objectId(state, false));
        s << l("        state_") << stateName << l(" = new Scxml::ScxmlState(");
        if (state->parentState() && state->parentState() != table)
            s << "state_" << table->objectId(state->parentState(), false);
        else
            s << "this";
        s << l(");\n");
        s << l("        addId(QByteArray(\"") << stateName << l("\"), state_") << stateName
          << l(");\n");
        if (ScxmlState *sState = qobject_cast<ScxmlState *>(state)) {
            if (!sState->onEntryInstruction.statements.isEmpty()) {
                s << l("        QObject::connect(state_") << stateName
                  << l(", &QAbstractState::entered, this, &") << mainClassName << l("::onEnter_")
                  << stateName << l(");\n");
            }
            if (!sState->onExitInstruction.statements.isEmpty()) {
                s << l("        QObject::connect(state_") << stateName
                  << l(", &QAbstractState::exited, this, &") << mainClassName << l("::onExit_")
                  << stateName << l(");\n");
            }
        }
        return true;
    }, Q_NULLPTR, [this](QAbstractState *state) -> void {
        QString stateName = QString::fromUtf8(table->objectId(state, false));
        s << l("        state_") << stateName << l(" = new ")
          << b(state->metaObject()->className()) << l("(");
        if (state->parentState() && state->parentState() != table)
            s << "state_" << table->objectId(state->parentState(), false);
        else
            s << "this";
        s << l(");\n");
        s << l("        addId(QByteArray(\"") << stateName << l("\"), state_") << stateName
          << l(");\n");
        if (ScxmlFinalState *sState = qobject_cast<ScxmlFinalState *>(state)) {
            if (!sState->onEntryInstruction.statements.isEmpty()) {
                s << l("        QObject::connect(state_") << stateName
                  << l(", &QAbstractState::entered, this, &") << mainClassName << l("::onEnter_")
                  << stateName << l(");\n");
            }
            if (!sState->onExitInstruction.statements.isEmpty()) {
                s << l("        QObject::connect(state_") << stateName
                  << l(", &QAbstractState::exited, this, &") << mainClassName << l("::onExit_")
                  << stateName << l(");\n");
            }
        }
    });
    if (table->initialState()) {
        s << l("\n        setInitialState(state_")
          << table->objectId(table->initialState(), true) << l(");\n\n");
    }
    loopOnSubStates(table, [this](QState *state) -> bool {
        QByteArray stateName = table->objectId(state);
        if (state->childMode() == QState::ExclusiveStates && state->initialState()) {
            s << l("\n        state_") << stateName << l("->setInitialState(state_")
              << table->objectId(state->initialState(), true) << l(");\n\n");
        }
        for (int tIndex = 0; tIndex < state->transitions().size(); ++tIndex) {
            QAbstractTransition *t = state->transitions().at(tIndex);
            if (Scxml::ScxmlTransition *scTransition = qobject_cast<Scxml::ScxmlTransition *>(t)) {
                s << l("        {\n");
                s << l("            QList<QByteArray> eventSelector;\n");
                if (!scTransition->eventSelector.isEmpty()) {
                    s << l("            eventSelector");
                    bool first = true;
                    foreach (const QByteArray &eSelector, scTransition->eventSelector) {
                        if (!first)
                            s << l("\n               ");
                        first = false;
                        s << l(" << QByteArray(\"") << cEscape(eSelector) << l("\")");
                    }
                    s << l(";\n");
                }
                QByteArray sourceState = table->objectId(t->sourceState(), true);
                if (sourceState.isEmpty())
                    sourceState = QByteArray("0");
                else
                    sourceState.prepend(QByteArray("state_"));
                if (scTransition->conditionalExp.isEmpty()) {
                    s << l("            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(")
                      << sourceState << l(", eventSelector);\n");
                    if (!scTransition->instructionsOnTransition.statements.isEmpty()) {
                        s << l("            QObject::connect(transition, &QAbstractTransition::triggered, this, &")
                          << mainClassName << l("::onTransition_") << tIndex << l("_") << stateName
                          << l(");\n");
                    }
                } else {
                    s << l("            Scxml::ScxmlBaseTransition *transition = new T_")
                      << tIndex << l("_") << stateName << l("(")
                      << sourceState << l(", eventSelector);\n");
                }
                QList<QAbstractState *> targetStates = scTransition->targetStates();
                if (targetStates.size() == 1) {
                    s << l("            transition.setTargetState(state_")
                      << table->objectId(targetStates.first(), true) << l(");\n");
                } else if (targetStates.size() > 1) {
                    s << l("            transition.setTargetStates(QList<QAbstractState *>() ");
                    foreach (QAbstractState *tState, targetStates)
                        s << l("\n                << state_") << table->objectId(tState, true);
                    s << l(");\n");
                }
                s << l("            transition->init();");
                s << l("        }\n");
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    s << l("        return true;\n");
    s << l("    }\n");
}

QByteArray CppDumper::cEscape(const QByteArray &str)
{ // should handle even trigraphs?
    QByteArray res;
    int lastI = 0;
    for (int i = 0; i < str.length(); ++i) {
        unsigned char c = str.at(i);
        if (c < ' ' || c == '\\' || c == '\"') {
            res.append(str.mid(lastI, i - lastI));
            lastI = i + 1;
            if (c == '\\') {
                res.append("\\\\");
            } else if (c == '\"') {
                res.append("\"");
            } else {
                char buf[4];
                buf[0] = '\\';
                buf[3] = '0' + (c & 0x7);
                c >>= 3;
                buf[2] = '0' + (c & 0x7);
                c >>= 3;
                buf[1] = '0' + (c & 0x7);
                res.append(&buf[0], 4);
            }
        }
    }
    if (lastI != 0) {
        res.append(str.mid(lastI));
        return res;
    }
    return str;
}

QString CppDumper::cEscape(const QString &str)
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
                res.append(QLatin1String("\""));
            } else {
                char buf[6];
                ushort cc = c.unicode();
                buf[0] = '\\';
                buf[1] = 'u';
                for (int i = 0; i < 4; ++i) {
                    buf[5 - i] = '0' + (cc & 0xF);
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

} // namespace Scxml
