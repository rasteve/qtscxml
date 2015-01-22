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

#include "scxmldumper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace Scxml {

namespace {
using namespace ExecutableContent;
class SCXML_EXPORT DumpInstructionVisitor : public InstructionVisitor {
public:
    DumpInstructionVisitor(ScxmlDumper &dumper) : s(dumper) { }
protected:
    void visitRaise(Raise *r) Q_DECL_OVERRIDE {
        s.writeStartElement("raise");
        s.writeAttribute("event", r->event);
        s.writeEndElement();
    }

    void visitSend(Send *send) Q_DECL_OVERRIDE {
        s.writeStartElement("send");
        if (!send->eventexpr.isEmpty())
            s.writeAttribute("eventexpr", send->eventexpr);
        else if (!send->event.isEmpty())
            s.writeAttribute("event", send->event);
        if (!send->targetexpr.isEmpty())
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
        if (!send->delayexpr.isEmpty())
            s.writeAttribute("delayexpr", send->delayexpr);
        else if (!send->delay.isEmpty())
            s.writeAttribute("delay", send->delay);
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
        }
        s.writeEndElement();
    }

    void visitLog(Log *log) Q_DECL_OVERRIDE {
        s.writeStartElement("log");
        if (!log->label.isEmpty())
            s.writeAttribute("label", log->label);
        if (!log->expr.isEmpty())
            s.writeAttribute("expr", log->expr);
        s.writeEndElement();
    }

    void visitJavaScript(JavaScript *script) Q_DECL_OVERRIDE {
        s.writeStartElement("script");
        if (!script->src.isEmpty())
            s.writeAttribute("src", script->src);
        else if (!script->source.isEmpty())
            s.s.writeCharacters(script->source); // escape? use CDATA?
        s.writeEndElement();
    }

    void visitAssignJson(AssignJson *assign) Q_DECL_OVERRIDE {
        s.writeStartElement("assign");
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
            QString json = doc.toJson(QJsonDocument::Compact);
            int from = json.indexOf(QLatin1Char('['));
            int to = json.lastIndexOf(QLatin1Char(']'));
            s.s.writeCharacters(json.mid(from + 1, to - from - 1));
            break;
        }
        case QJsonValue::Array: {
            QJsonDocument doc(assign->value.toArray());
            s.s.writeCharacters(doc.toJson());
            break;
        }
        case QJsonValue::Object: {
            QJsonDocument doc(assign->value.toObject());
            s.s.writeCharacters(doc.toJson());
            break;
        }
        case QJsonValue::Undefined:
            s.s.writeCharacters(QLatin1String("undefined"));
            break;
        }
        s.writeEndElement();
    }

    void visitAssignExpression(AssignExpression *assign) Q_DECL_OVERRIDE {
        s.writeStartElement("assign");
        s.writeAttribute("location", assign->location);
        if (assign->content)
            assign->content->dump(s.s);
        else if (!assign->expression.isEmpty())
            s.writeAttribute("expr", assign->expression);
        s.writeEndElement();
    }

    void visitCancel(Cancel *c) Q_DECL_OVERRIDE {
        s.writeStartElement("cancel");
        if (!c->sendidexpr.isEmpty())
            s.writeAttribute("sendidexpr", c->sendidexpr);
        else if (!c->sendid.isEmpty())
            s.writeAttribute("sendid", c->sendid);
    }

    bool visitInvoke(Invoke *invoke) Q_DECL_OVERRIDE {
        s.writeStartElement("invoke");
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
        s.writeEndElement();
        return false;
    }

    void endVisitInvoke(Invoke *) Q_DECL_OVERRIDE { }

    bool visitIf(If *ifI) Q_DECL_OVERRIDE {
        s.writeStartElement("if");
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
        s.writeEndElement();
        return false;
    }

    void endVisitIf(If *) Q_DECL_OVERRIDE { }
    bool visitForeach(Foreach *foreachI) {
        s.writeStartElement("foreach");
        s.writeAttribute("array", foreachI->array);
        s.writeAttribute("item", foreachI->item);
        if (!foreachI->index.isEmpty())
            s.writeAttribute("index", foreachI->index);
        return true;
    }

    void endVisitForeach(Foreach *) Q_DECL_OVERRIDE {
        s.writeEndElement();
    }
    bool visitSequence(InstructionSequence *) Q_DECL_OVERRIDE { return true; }
    void endVisitSequence(InstructionSequence *) Q_DECL_OVERRIDE { }

    ScxmlDumper &s;
};
} // anonymous namespace

void ScxmlDumper::dump(StateTable *table)
{
    this->table = table;
    s.writeStartDocument();
    scxmlStart();
    loopOnSubStates(table, [this](QState *state) -> bool { return this->enterState(state); },
    [this](QState *state) { this->exitState(state); },
    [this](QAbstractState *state) { this->inAbstractState(state); });
    s.writeEndElement(); // scxml
    s.writeEndDocument();
}

void ScxmlDumper::scxmlStart() {
    writeStartElement("scxml");
    writeAttribute("xmlns", "http://www.w3.org/2005/07/scxml");
    writeAttribute("version", "1.0");
    QString datamodel;
    switch (table->dataModel()) {
    case StateTable::Javascript:
        datamodel = QStringLiteral("ecmascript");
        break;
    case StateTable::Json:
        datamodel = QStringLiteral("json");
        break;
    case StateTable::Xml:
        datamodel = QStringLiteral("xpath");
        break;
    case StateTable::None:
        datamodel = QStringLiteral("none");
        break;
    }
    if (!datamodel.isEmpty())
        writeAttribute("datamodel", datamodel);
    QString binding;
    switch (table->dataBinding()) {
    case StateTable::EarlyBinding:
        binding = QStringLiteral("early");
        break;
    case StateTable::LateBinding:
        binding = QStringLiteral("late");
        break;
    }
    if (!binding.isEmpty())
        writeAttribute("binding", binding);
    QString name;
    if (!table->_name.isEmpty())
        name = table->_name;
    else if (!table->objectName().isEmpty())
        name = table->objectName();
    if (!name.isEmpty())
        writeAttribute("name", name);
}

void ScxmlDumper::dumpTransition(QAbstractTransition *transition)
{
    writeStartElement("transition");
    QString event = QStringLiteral("unknown");
    QString cond;
    if (ScxmlTransition *t = qobject_cast<ScxmlTransition *>(transition)) {
        event = t->eventSelector;
        cond = t->conditionalExp;
    }
    if (!event.isEmpty())
        writeAttribute("event", event);
    if (!cond.isEmpty())
        writeAttribute("cond", cond);
    QList<QAbstractState *> targets = transition->targetStates();
    QStringList targetNames;
    for (int i = 0; i < targets.size(); ++i)
        targetNames[i] = table->objectId(targets.at(i));
    if (!targetNames.isEmpty())
        writeAttribute("target", targetNames.join(QLatin1Char(' ')));
    writeAttribute("type", "internal");
}

void ScxmlDumper::dumpInstruction(Instruction *instruction)
{
    if (!instruction)
        return;
    DumpInstructionVisitor iDumper(*this);
    iDumper.accept(instruction);
}

bool ScxmlDumper::enterState(QState *state)
{
    if (state->parentState() && state->parentState()->initialState() == state)
        if (ScxmlInitialState *i = qobject_cast<ScxmlInitialState *>(state)) {
        writeStartElement("initial");
        if (!i->transitions().isEmpty())
            dumpTransition(i->transitions().first());
        writeEndElement();
        if (i->transitions().size() != 1) {
            qCWarning(scxmlLog) << "initial element" << i->stateLocation()
                                << " should have exactly one transition";
        }
        return false;
    }
    switch (state->childMode()) {
    case QState::ExclusiveStates:
        writeStartElement("state");
        break;
    case QState::ParallelStates:
        writeStartElement("parallel");
        break;
    }
    writeAttribute("id", table->objectId(state));
    if (state->initialState())
        writeAttribute("initial", table->objectId(state->initialState()));
    if (ScxmlState *ss = qobject_cast<ScxmlState *>(state)) {
        if (!ss->onEntryInstruction.statements.isEmpty()) {
            writeStartElement("onentry");
            dumpInstruction(&ss->onEntryInstruction);
            writeEndElement();
        }
    }
    return true;
}

void ScxmlDumper::exitState(QState *state)
{
    Q_UNUSED(state);
    Q_ASSERT(false);
}

void ScxmlDumper::inAbstractState(QAbstractState *state)
{
    Q_UNUSED(state);
    Q_ASSERT(false);
}


}

