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

#include "executablecontent_p.h"
#include "scxmlevent_p.h"

using namespace Scxml;
using namespace Scxml::ExecutableContent;

static int parseTime(const QString &t, bool *ok = 0)
{
    if (t.isEmpty()) {
        if (ok)
            *ok = false;
        return -1;
    }
    bool negative = false;
    int startPos = 0;
    if (t[0] == QLatin1Char('-')) {
        negative = true;
        ++startPos;
    } else if (t[0] == QLatin1Char('+')) {
        ++startPos;
    }
    int pos = startPos;
    for (int endPos = t.length(); pos < endPos; ++pos) {
        auto c = t[pos];
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            break;
    }
    if (pos == startPos) {
        if (ok) *ok = false;
        return -1;
    }
    int value = t.midRef(startPos, pos - startPos).toInt(ok);
    if (ok && !*ok) return -1;
    if (t.length() == pos + 1 && t[pos] == QLatin1Char('s')) {
        value *= 1000;
    } else if (t.length() != pos + 2 || t[pos] != QLatin1Char('m') || t[pos + 1] != QLatin1Char('s')) {
        if (ok) *ok = false;
        return -1;
    }
    return negative ? -value : value;
}

ExecutionEngine::ExecutionEngine(StateTable *table)
    : table(table)
{}

void ExecutionEngine::setStringTable(const QVector<QString> &strings)
{
    this->strings = strings;
}

QString ExecutionEngine::string(StringId id) const
{
    return strings.at(id);
}

void ExecutionEngine::setByteArrayTable(const QVector<QByteArray> &byteArrays)
{
    this->byteArrays = byteArrays;
}

QByteArray ExecutionEngine::byteArray(ByteArrayId id) const
{
    return byteArrays.at(id);
}

void ExecutionEngine::setInstructions(const QVector<qint32> &instructions)
{
    this->instructions = instructions;
}

bool ExecutionEngine::execute(ContainerId id)
{
    if (id == NoInstruction)
        return true;

    qint32 *ip = &instructions[id];
    return step(ip);
}

const DoneData *ExecutionEngine::doneData(ContainerId id) const
{
    if (id == NoInstruction)
        return nullptr;
    return reinterpret_cast<const DoneData *>(&instructions[id]);
}

bool ExecutionEngine::step(Instructions &ip)
{
    auto dataModel = table->dataModel();
    auto executionEngine = table->executionEngine();

    auto instr = reinterpret_cast<Instruction *>(ip);
    switch (instr->instructionType) {
    case Instruction::Sequence: {
        qDebug() << "Executing sequence step";
        InstructionSequence *sequence = reinterpret_cast<InstructionSequence *>(instr);
        ip = sequence->instructions();
        Instructions end = ip + sequence->entryCount;
        while (ip < end) {
            if (!step(ip)) {
                ip = end;
                qDebug() << "Finished sequence step UNsuccessfully";
                return false;
            }
        }
        qDebug() << "Finished sequence step successfully";
        return true;
    }

    case Instruction::Sequences: {
        qDebug() << "Executing sequences step";
        InstructionSequences *sequences = reinterpret_cast<InstructionSequences *>(instr);
        ip += sequences->size();
        for (int i = 0; i != sequences->sequenceCount; ++i) {
            Instructions sequence = sequences->at(i);
            step(sequence);
        }
        qDebug() << "Finished sequences step";
        return true;
    }

    case Instruction::Send: {
        qDebug() << "Executing send step";
        Send *send = reinterpret_cast<Send *>(instr);
        ip += send->size();

        QString delay = executionEngine->string(send->delay);
        if (send->delayexpr != DataModel::NoEvaluator) {
            bool ok = false;
            delay = table->dataModel()->evaluateToString(send->delayexpr, &ok);
            if (!ok)
                return false;
        }

        ScxmlEvent *event = EventBuilder(table, *send).buildEvent();
        if (!event)
            return false;

        if (delay.isEmpty()) {
            table->submitEvent(event);
        } else {
            int msecs = parseTime(delay);
            if (msecs >= 0) {
                table->submitDelayedEvent(msecs, event);
            } else {
                qCDebug(scxmlLog) << "failed to parse delay time" << delay;
                return false;
            }
        }

        return true;
    }

    case Instruction::JavaScript: {
        qDebug() << "Executing javascript step";
        JavaScript *javascript = reinterpret_cast<JavaScript *>(instr);
        ip += javascript->size();
        bool ok = true;
        dataModel->evaluateToVoid(javascript->go, &ok);
        return ok;
    }

    case Instruction::If: {
        qDebug() << "Executing if step";
        If *_if = reinterpret_cast<If *>(instr);
        ip += _if->size();
        auto blocks = _if->blocks();
        for (qint32 i = 0; i < _if->conditions.count; ++i) {
            bool ok = true;
            if (dataModel->evaluateToBool(_if->conditions.at(i), &ok) && ok) {
                Instructions block = blocks->at(i);
                bool res = step(block);
                qDebug()<<"Finished if step";
                return res;
            }
        }

        if (_if->conditions.count < blocks->sequenceCount) {
            Instructions block = blocks->at(_if->conditions.count);
            return step(block);
        }

        return true;
    }

    case Instruction::Foreach: {
        qDebug() << "Executing foreach step";
        Foreach *foreach = reinterpret_cast<Foreach *>(instr);
        Instructions loopStart = foreach->blockstart();
        ip += foreach->size();
        bool ok = true;
        bool evenMoreOk = dataModel->evaluateForeach(foreach->doIt, &ok, [this,loopStart]()-> bool {
            Instructions loop = loopStart;
            return step(loop);
        });
        return ok && evenMoreOk;
    }

    case Instruction::Raise: {
        qDebug() << "Executing raise step";
        Raise *raise = reinterpret_cast<Raise *>(instr);
        ip += raise->size();
        auto event = executionEngine->byteArray(raise->event);
        table->submitEvent(event, QVariantList(), QStringList(), ScxmlEvent::Internal);
        return true;
    }

    case Instruction::Log: {
        qDebug() << "Executing log step";
        Log *log = reinterpret_cast<Log *>(instr);
        ip += log->size();
        bool ok = true;
        QString str = dataModel->evaluateToString(log->expr, &ok);
        if (ok)
            table->doLog(executionEngine->string(log->label), str);
        return ok;
    }

    case Instruction::Cancel: {
        qDebug() << "Executing cancel step";
        Cancel *cancel = reinterpret_cast<Cancel *>(instr);
        ip += cancel->size();
        QByteArray e = executionEngine->byteArray(cancel->sendid);
        bool ok = true;
        if (cancel->sendidexpr != DataModel::NoEvaluator)
            e = dataModel->evaluateToString(cancel->sendidexpr, &ok).toUtf8();
        if (ok && !e.isEmpty())
            table->cancelDelayedEvent(e);
        return ok;
    }

    case Instruction::Invoke:
        Q_UNIMPLEMENTED();
        Q_UNREACHABLE();
        return false;

    case Instruction::Assign: {
        qDebug() << "Executing assign step";
        Assign *assign = reinterpret_cast<Assign *>(instr);
        ip += assign->size();
        bool ok = true;
        dataModel->evaluateToVoid(assign->expression, &ok);
        return ok;
    }

    default:
        Q_UNREACHABLE();
        return false;
    }
}

Builder::Builder()
{
    m_activeSequences.reserve(4);
}

bool Builder::visit(DocumentModel::Send *node)
{
    auto instr = m_instructions.add<Send>(Send::calculateExtraSize(node->params.size(), node->namelist.size()));
    instr->instructionLocation = createContext(QStringLiteral("send"));
    instr->event = m_byteArrayTable.add(node->event.toUtf8());
    instr->eventexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr);
    instr->type = m_stringTable.add(node->type);
    instr->typeexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("typeexpr"), node->typeexpr);
    instr->target = m_stringTable.add(node->target);
    instr->targetexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("targetexpr"), node->targetexpr);
    instr->id = m_byteArrayTable.add(node->id.toUtf8());
    instr->idLocation = m_stringTable.add(node->idLocation);
    instr->delay = m_stringTable.add(node->delay);
    instr->delayexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("delayexpr"), node->delayexpr);
    instr->content = m_stringTable.add(node->content);
    generate(&instr->namelist, node->namelist);
    generate(instr->params(), node->params);
    return false;
}

void Builder::visit(DocumentModel::Raise *node)
{
    auto instr = m_instructions.add<Raise>();
    instr->event = m_byteArrayTable.add(node->event.toUtf8());
}

void Builder::visit(DocumentModel::Log *node)
{
    auto instr = m_instructions.add<Log>();
    instr->label = m_stringTable.add(node->label);
    instr->expr = createEvaluatorString(QStringLiteral("log"), QStringLiteral("expr"), node->expr);
}

void Builder::visit(DocumentModel::Script *node)
{
    auto instr = m_instructions.add<JavaScript>();
    auto ctxt = createContext(QStringLiteral("script"), QStringLiteral("source"), node->content);
    instr->go = dataModel()->createScriptEvaluator(node->content, ctxt);
}

void Builder::visit(DocumentModel::Assign *node)
{
    auto instr = m_instructions.add<Assign>();
    qDebug()<<"...:" <<node->location<<"="<<node->expr;
    auto ctxt = createContext(QStringLiteral("assign"), QStringLiteral("expr"), node->expr);
    instr->expression = dataModel()->createAssignmentEvaluator(node->location, node->expr, ctxt);
}

bool Builder::visit(DocumentModel::If *node)
{
    auto instr = m_instructions.add<If>(node->conditions.size());
    instr->conditions.count = node->conditions.size();
    auto it = instr->conditions.data();
    QString tag = QStringLiteral("if");
    for (int i = 0, ei = node->conditions.size(); i != ei; ++i) {
        *it++ = createEvaluatorBool(tag, QStringLiteral("cond"), node->conditions.at(i));
        if (i == 0) {
            tag = QStringLiteral("elif");
        }
    }
    auto outSequences = m_instructions.add<InstructionSequences>();
    generate(outSequences, node->blocks);
    return false;
}

bool Builder::visit(DocumentModel::Foreach *node)
{
    auto instr = m_instructions.add<Foreach>();
    auto ctxt = createContextString(QStringLiteral("foreach"));
    instr->doIt = dataModel()->createForeachEvaluator(node->array, node->item, node->index, ctxt);
    startSequence(&instr->block);
    visit(&node->block);
    endSequence();
    return false;
}

void Builder::visit(DocumentModel::Cancel *node)
{
    auto instr = m_instructions.add<Cancel>();
    instr->sendid = m_byteArrayTable.add(node->sendid.toUtf8());
    instr->sendidexpr = createEvaluatorString(QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
}

bool Builder::visit(DocumentModel::Invoke *)
{
    Q_UNIMPLEMENTED();
    return false;
}

QVector<QString> Builder::stringTable()
{
    m_stringTable.squeeze();
    return m_stringTable;
}

QVector<QByteArray> Builder::byteArrayTable()
{
    m_byteArrayTable.squeeze();
    return m_byteArrayTable;
}

ContainerId Builder::generate(DocumentModel::DoneData *node)
{
    auto id = m_instructions.newContainerId();
    auto doneData = m_instructions.add<DoneData>(node->params.size() * Param::calculateSize());
    doneData->contents = m_stringTable.add(node->contents);
    doneData->expr = createEvaluatorString(QStringLiteral("donedata"), QStringLiteral("expr"), node->expr);
    generate(&doneData->params, node->params);
    return id;
}

StringId Builder::createContext(const QString &instrName)
{
    return m_stringTable.add(createContextString(instrName));
}

void Builder::generate(ContainerId *id, const DocumentModel::InstructionSequences &inSequences)
{
    if (inSequences.isEmpty())
        return;

    *id = m_instructions.newContainerId();
    auto outSequences = m_instructions.add<InstructionSequences>();
    generate(outSequences, inSequences);
}

void Builder::generate(Array<Param> *out, const QVector<DocumentModel::Param *> &in)
{
    out->count = in.size();
    Param *it = out->data();
    foreach (DocumentModel::Param *f, in) {
        it->name = m_stringTable.add(f->name);
        it->expr = createEvaluatorVariant(QStringLiteral("param"), QStringLiteral("expr"), f->expr);
        it->location = m_stringTable.add(f->location);
        ++it;
    }
}

void Builder::generate(InstructionSequences *outSequences, const DocumentModel::InstructionSequences &inSequences)
{
    int sequencesOffset = m_instructions.offset(outSequences);
    int sequenceCount = 0;
    int entryCount = 0;
    foreach (DocumentModel::InstructionSequence *sequence, inSequences) {
        ++sequenceCount;
        startNewSequence();
        visit(sequence);
        entryCount += endSequence()->size();
    }
    outSequences = m_instructions.at<InstructionSequences>(sequencesOffset);
    outSequences->sequenceCount = sequenceCount;
    outSequences->entryCount = entryCount;
}

void Builder::generate(Array<StringId> *out, const QStringList &in)
{
    out->count = in.size();
    StringId *it = out->data();
    foreach (const QString &str, in) {
        *it++ = m_stringTable.add(str);
    }
}

ContainerId Builder::startNewSequence()
{
    auto id = m_instructions.newContainerId();
    auto sequence = m_instructions.add<InstructionSequence>();
    startSequence(sequence);
    return id;
}

void Builder::startSequence(InstructionSequence *sequence)
{
    SequenceInfo info;
    info.location = m_instructions.offset(sequence);
    info.entryCount = 0;
    m_activeSequences.push_back(info);
    m_instructions.setSequenceInfo(&m_activeSequences.last());
    sequence->instructionType = Instruction::Sequence;
    sequence->entryCount = -1; // checked in endSequence
    qDebug()<<"starting sequence with depth"<<m_activeSequences.size();
}

InstructionSequence *Builder::endSequence()
{
    SequenceInfo info = m_activeSequences.back();
    m_activeSequences.pop_back();
    m_instructions.setSequenceInfo(m_activeSequences.isEmpty() ? nullptr : &m_activeSequences.last());

    auto sequence = m_instructions.at<InstructionSequence>(info.location);
    Q_ASSERT(sequence->entryCount == -1); // set in startSequence
    sequence->entryCount = info.entryCount;
    if (!m_activeSequences.isEmpty())
        m_activeSequences.last().entryCount += info.entryCount;
    qDebug()<<"finished sequence with"<<info.entryCount<<"bytes, depth" << (m_activeSequences.size()+1);
    return sequence;
}

DataModel::EvaluatorId Builder::createEvaluatorString(const QString &instrName, const QString &attrName, const QString &expr) const
{
    if (!expr.isEmpty()) {
        QString loc = createContext(instrName, attrName, expr);
        return dataModel()->createToStringEvaluator(expr, loc);
    }

    return DataModel::NoEvaluator;
}

DataModel::EvaluatorId Builder::createEvaluatorBool(const QString &instrName, const QString &attrName, const QString &cond) const
{
    if (!cond.isEmpty()) {
        QString loc = createContext(instrName, attrName, cond);
        return dataModel()->createToBoolEvaluator(cond, loc);
    }

    return DataModel::NoEvaluator;
}

DataModel::EvaluatorId Builder::createEvaluatorVariant(const QString &instrName, const QString &attrName, const QString &cond) const
{
    if (!cond.isEmpty()) {
        QString loc = createContext(instrName, attrName, cond);
        return dataModel()->createToVariantEvaluator(cond, loc);
    }

    return DataModel::NoEvaluator;
}
