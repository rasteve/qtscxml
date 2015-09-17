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
#include "scxmlparser_p.h"
#include "scxmlevent_p.h"

using namespace Scxml;
using namespace Scxml::QScxmlExecutableContent;

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

ExecutionEngine::ExecutionEngine(StateMachine *table)
    : table(table)
{
    Q_ASSERT(table);
}

bool ExecutionEngine::execute(ContainerId id, const QVariant &extraData)
{
    Q_ASSERT(table);

    if (id == NoInstruction)
        return true;

    qint32 *ip = table->tableData()->instructions() + id;
    this->extraData = extraData;
    bool result = step(ip);
    this->extraData = QVariant();
    return result;
}

bool ExecutionEngine::step(Instructions &ip)
{
    auto dataModel = table->dataModel();
    auto tableData = table->tableData();

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

        QString delay = tableData->string(send->delay);
        if (send->delayexpr != NoEvaluator) {
            bool ok = false;
            delay = table->dataModel()->evaluateToString(send->delayexpr, &ok);
            if (!ok)
                return false;
        }

        QScxmlEvent *event = EventBuilder(table, *send).buildEvent();
        if (!event)
            return false;

        if (!delay.isEmpty()) {
            int msecs = parseTime(delay);
            if (msecs >= 0) {
                event->setDelay(msecs);
            } else {
                qCDebug(scxmlLog) << "failed to parse delay time" << delay;
                return false;
            }
        }

        table->routeEvent(event);
        return true;
    }

    case Instruction::JavaScript: {
        qDebug() << "Executing script step";
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
        class LoopBody: public QScxmlDataModel::ForeachLoopBody // If only we could put std::function in public API, we could use a lambda here. Alas....
        {
            ExecutionEngine *engine;
            const Instructions loopStart;

        public:
            LoopBody(ExecutionEngine *engine, const Instructions loopStart)
                : engine(engine)
                , loopStart(loopStart)
            {}

            bool run() Q_DECL_OVERRIDE
            {
                Instructions ip = loopStart;
                return engine->step(ip);
            }
        };

        qDebug() << "Executing foreach step";
        Foreach *foreach = reinterpret_cast<Foreach *>(instr);
        Instructions loopStart = foreach->blockstart();
        ip += foreach->size();
        bool ok = true;
        LoopBody body(this, loopStart);
        bool evenMoreOk = dataModel->evaluateForeach(foreach->doIt, &ok, &body);
        return ok && evenMoreOk;
    }

    case Instruction::Raise: {
        qDebug() << "Executing raise step";
        Raise *raise = reinterpret_cast<Raise *>(instr);
        ip += raise->size();
        auto name = tableData->byteArray(raise->event);
        auto event = new QScxmlEvent;
        event->setName(name);
        event->setEventType(QScxmlEvent::InternalEvent);
        table->submitEvent(event);
        return true;
    }

    case Instruction::Log: {
        qDebug() << "Executing log step";
        Log *log = reinterpret_cast<Log *>(instr);
        ip += log->size();
        bool ok = true;
        QString str = dataModel->evaluateToString(log->expr, &ok);
        if (ok)
            table->doLog(tableData->string(log->label), str);
        return ok;
    }

    case Instruction::Cancel: {
        qDebug() << "Executing cancel step";
        Cancel *cancel = reinterpret_cast<Cancel *>(instr);
        ip += cancel->size();
        QByteArray e = tableData->byteArray(cancel->sendid);
        bool ok = true;
        if (cancel->sendidexpr != NoEvaluator)
            e = dataModel->evaluateToString(cancel->sendidexpr, &ok).toUtf8();
        if (ok && !e.isEmpty())
            table->cancelDelayedEvent(e);
        return ok;
    }

    case Instruction::Assign: {
        qDebug() << "Executing assign step";
        Assign *assign = reinterpret_cast<Assign *>(instr);
        ip += assign->size();
        bool ok = true;
        dataModel->evaluateAssignment(assign->expression, &ok);
        return ok;
    }

    case Instruction::Initialize: {
        qDebug() << "Executing initialize step";
        Initialize *init = reinterpret_cast<Initialize *>(instr);
        ip += init->size();
        bool ok = true;
        dataModel->evaluateInitialization(init->expression, &ok);
        return ok;
    }

    case Instruction::DoneData: {
        qDebug() << "Executing DoneData step";
        DoneData *doneData = reinterpret_cast<DoneData *>(instr);

        QString eventName = QStringLiteral("done.state.") + extraData.toString();
        EventBuilder event(table, eventName, doneData);
        qDebug() << "submitting event" << eventName;
        table->submitEvent(event());
        return true;
    }

    default:
        Q_UNREACHABLE();
        return false;
    }
}

Builder::Builder()
    : m_initialSetup(QScxmlExecutableContent::NoInstruction)
    , m_isCppDataModel(false)
{
    m_activeSequences.reserve(4);
}

bool Builder::visit(DocumentModel::Send *node)
{
    auto instr = m_instructions.add<Send>(Send::calculateExtraSize(node->params.size(), node->namelist.size()));
    instr->instructionLocation = createContext(QStringLiteral("send"));
    instr->event = addByteArray(node->event.toUtf8());
    instr->eventexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr);
    instr->type = addString(node->type);
    instr->typeexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("typeexpr"), node->typeexpr);
    instr->target = addString(node->target);
    instr->targetexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("targetexpr"), node->targetexpr);
    instr->id = addByteArray(node->id.toUtf8());
    instr->idLocation = addString(node->idLocation);
    instr->delay = addString(node->delay);
    instr->delayexpr = createEvaluatorString(QStringLiteral("send"), QStringLiteral("delayexpr"), node->delayexpr);
    instr->content = addString(node->content);
    generate(&instr->namelist, node->namelist);
    generate(instr->params(), node->params);
    return false;
}

void Builder::visit(DocumentModel::Raise *node)
{
    auto instr = m_instructions.add<Raise>();
    instr->event = addByteArray(node->event.toUtf8());
}

void Builder::visit(DocumentModel::Log *node)
{
    auto instr = m_instructions.add<Log>();
    instr->label = addString(node->label);
    instr->expr = createEvaluatorString(QStringLiteral("log"), QStringLiteral("expr"), node->expr);
}

void Builder::visit(DocumentModel::Script *node)
{
    auto instr = m_instructions.add<JavaScript>();
    instr->go = createEvaluatorVoid(QStringLiteral("script"), QStringLiteral("source"), node->content);
}

void Builder::visit(DocumentModel::Assign *node)
{
    auto instr = m_instructions.add<Assign>();
//    qDebug()<<"...:" <<node->location<<"="<<node->expr;
    auto ctxt = createContext(QStringLiteral("assign"), QStringLiteral("expr"), node->expr);
    instr->expression = addAssignment(node->location, node->expr, ctxt);
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
    instr->doIt = addForeach(node->array, node->item, node->index, ctxt);
    startSequence(&instr->block);
    visit(&node->block);
    endSequence();
    return false;
}

void Builder::visit(DocumentModel::Cancel *node)
{
    auto instr = m_instructions.add<Cancel>();
    instr->sendid = addByteArray(node->sendid.toUtf8());
    instr->sendidexpr = createEvaluatorString(QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
}

ContainerId Builder::generate(const DocumentModel::DoneData *node)
{
    auto id = m_instructions.newContainerId();
    DoneData *doneData;
    if (node) {
        doneData = m_instructions.add<DoneData>(node->params.size() * Param::calculateSize());
        doneData->contents = addString(node->contents);
        doneData->expr = createEvaluatorString(QStringLiteral("donedata"), QStringLiteral("expr"), node->expr);
        generate(&doneData->params, node->params);
    } else {
        doneData = m_instructions.add<DoneData>();
        doneData->contents = NoString;
        doneData->expr = NoEvaluator;
        doneData->params.count = 0;
    }
    doneData->location = createContext(QStringLiteral("final"));
    return id;
}

StringId Builder::createContext(const QString &instrName)
{
    return addString(createContextString(instrName));
}

void Builder::generate(const QVector<DocumentModel::DataElement *> &dataElements)
{
    foreach (DocumentModel::DataElement *el, dataElements) {
        auto ctxt = createContext(QStringLiteral("data"), QStringLiteral("expr"), el->expr);
        auto evaluator = addDataElement(el->id, el->expr, ctxt);
        if (evaluator != NoEvaluator) {
            auto instr = m_instructions.add<QScxmlExecutableContent::Initialize>();
            instr->expression = evaluator;
        }
    }
}

ContainerId Builder::generate(const DocumentModel::InstructionSequences &inSequences)
{
    if (inSequences.isEmpty())
        return NoInstruction;

    auto id = m_instructions.newContainerId();
    auto outSequences = m_instructions.add<InstructionSequences>();
    generate(outSequences, inSequences);
    return id;
}

void Builder::generate(Array<Param> *out, const QVector<DocumentModel::Param *> &in)
{
    out->count = in.size();
    Param *it = out->data();
    foreach (DocumentModel::Param *f, in) {
        it->name = addString(f->name);
        it->expr = createEvaluatorVariant(QStringLiteral("param"), QStringLiteral("expr"), f->expr);
        it->location = addString(f->location);
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
        *it++ = addString(str);
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
//    qDebug()<<"starting sequence with depth"<<m_activeSequences.size();
}

InstructionSequence *Builder::endSequence()
{
    SequenceInfo info = m_activeSequences.back();
    m_activeSequences.pop_back();
    m_instructions.setSequenceInfo(m_activeSequences.isEmpty() ? Q_NULLPTR : &m_activeSequences.last());

    auto sequence = m_instructions.at<InstructionSequence>(info.location);
    Q_ASSERT(sequence->entryCount == -1); // set in startSequence
    sequence->entryCount = info.entryCount;
    if (!m_activeSequences.isEmpty())
        m_activeSequences.last().entryCount += info.entryCount;
//    qDebug()<<"finished sequence with"<<info.entryCount<<"bytes, depth" << (m_activeSequences.size()+1);
    return sequence;
}

EvaluatorId Builder::createEvaluatorString(const QString &instrName, const QString &attrName, const QString &expr)
{
    if (!expr.isEmpty()) {
        if (isCppDataModel()) {
            auto id = m_evaluators.add(QScxmlEvaluatorInfo(), false);
            m_stringEvaluators.insert(id, expr);
            return id;
        } else {
            QString loc = createContext(instrName, attrName, expr);
            return addEvaluator(expr, loc);
        }
    }

    return NoEvaluator;
}

EvaluatorId Builder::createEvaluatorBool(const QString &instrName, const QString &attrName, const QString &cond)
{
    if (!cond.isEmpty()) {
        if (isCppDataModel()) {
            auto id = m_evaluators.add(QScxmlEvaluatorInfo(), false);
            m_boolEvaluators.insert(id, cond);
            return id;
        } else {
            QString loc = createContext(instrName, attrName, cond);
            return addEvaluator(cond, loc);
        }
    }

    return NoEvaluator;
}

EvaluatorId Builder::createEvaluatorVariant(const QString &instrName, const QString &attrName, const QString &expr)
{
    if (!expr.isEmpty()) {
        if (isCppDataModel()) {
            auto id = m_evaluators.add(QScxmlEvaluatorInfo(), false);
            m_variantEvaluators.insert(id, expr);
            return id;
        } else {
            QString loc = createContext(instrName, attrName, expr);
            return addEvaluator(expr, loc);
        }
    }

    return NoEvaluator;
}

EvaluatorId Builder::createEvaluatorVoid(const QString &instrName, const QString &attrName, const QString &stuff)
{
    if (!stuff.isEmpty()) {
        if (isCppDataModel()) {
            auto id = m_evaluators.add(QScxmlEvaluatorInfo(), false);
            m_voidEvaluators.insert(id, stuff);
            return id;
        } else {
            QString loc = createContext(instrName, attrName, stuff);
            return addEvaluator(stuff, loc);
        }
    }

    return NoEvaluator;
}

DynamicTableData *Builder::tableData()
{
    auto td = new DynamicTableData;
    td->strings = m_stringTable.data();
    td->byteArrays = m_byteArrayTable.data();
    td->theInstructions = m_instructions.data();
    td->theEvaluators = m_evaluators.data();
    td->theAssignments = m_assignments.data();
    td->theForeaches = m_foreaches.data();
    td->theDataNameIds = m_dataIds;
    td->theInitialSetup = m_initialSetup;
    td->theName = m_name;
    return td;
}

QString DynamicTableData::string(StringId id) const
{
    return id == NoString ? QString() : strings.at(id);
}

QByteArray DynamicTableData::byteArray(ByteArrayId id) const
{
    return id == NoByteArray ? QByteArray() : byteArrays.at(id);
}

Instructions DynamicTableData::instructions() const
{
    return const_cast<Instructions>(theInstructions.data());
}

QScxmlEvaluatorInfo DynamicTableData::evaluatorInfo(EvaluatorId evaluatorId) const
{
    return theEvaluators[evaluatorId];
}

QScxmlAssignmentInfo DynamicTableData::assignmentInfo(EvaluatorId assignmentId) const
{
    return theAssignments[assignmentId];
}

QScxmlForeachInfo DynamicTableData::foreachInfo(EvaluatorId foreachId) const
{
    return theForeaches[foreachId];
}

StringId *DynamicTableData::dataNames(int *count) const
{
    Q_ASSERT(count);
    *count = theDataNameIds.size();
    return const_cast<StringId *>(theDataNameIds.data());
}

ContainerId DynamicTableData::initialSetup() const
{
    return theInitialSetup;
}

QString DynamicTableData::name() const
{
    return theName;
}

QVector<qint32> DynamicTableData::instructionTable() const
{
    return theInstructions;
}

QVector<QString> DynamicTableData::stringTable() const
{
    return strings;
}

QVector<QByteArray> DynamicTableData::byteArrayTable() const
{
    return byteArrays;
}

QVector<QScxmlEvaluatorInfo> DynamicTableData::evaluators() const
{
    return theEvaluators;
}

QVector<QScxmlAssignmentInfo> DynamicTableData::assignments() const
{
    return theAssignments;
}

QVector<QScxmlForeachInfo> DynamicTableData::foreaches() const
{
    return theForeaches;
}

StringIds DynamicTableData::allDataNameIds() const
{
    return theDataNameIds;
}
