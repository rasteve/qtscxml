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

#ifndef EXECUTABLECONTENT_P_H
#define EXECUTABLECONTENT_P_H

#include "executablecontent.h"
#include "scxmlparser.h"
#include "scxmlstatetable.h"

namespace Scxml {

namespace ExecutableContent {

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
        DoneData
    } instructionType;
};

struct SCXML_EXPORT DoneData: Instruction
{
    StringId location;
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

class Builder: public DocumentModel::NodeVisitor
{
public:
    Builder();

protected: // visitor
    using NodeVisitor::visit;

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE;
    void visit(DocumentModel::Raise *node) Q_DECL_OVERRIDE;
    void visit(DocumentModel::Log *node) Q_DECL_OVERRIDE;
    void visit(DocumentModel::Script *node) Q_DECL_OVERRIDE;
    void visit(DocumentModel::Assign *node) Q_DECL_OVERRIDE;
    bool visit(DocumentModel::If *node) Q_DECL_OVERRIDE;
    bool visit(DocumentModel::Foreach *node) Q_DECL_OVERRIDE;
    void visit(DocumentModel::Cancel *node) Q_DECL_OVERRIDE;
    bool visit(DocumentModel::Invoke *) Q_DECL_OVERRIDE;

protected:
    QVector<qint32> instructions() { return m_instructions.data(); }
    QVector<QString> stringTable();
    QVector<QByteArray> byteArrayTable();

    ContainerId generate(const DocumentModel::DoneData *node);
    StringId createContext(const QString &instrName);
    void generate(const QVector<DocumentModel::DataElement *> &dataElements);
    ContainerId generate(const DocumentModel::InstructionSequences &inSequences);
    void generate(Array<Param> *out, const QVector<DocumentModel::Param *> &in);
    void generate(InstructionSequences *outSequences, const DocumentModel::InstructionSequences &inSequences);
    void generate(Array<StringId> *out, const QStringList &in);
    ContainerId startNewSequence();
    void startSequence(InstructionSequence *sequence);
    InstructionSequence *endSequence();
    DataModel::EvaluatorId createEvaluatorString(const QString &instrName, const QString &attrName, const QString &expr);
    DataModel::EvaluatorId createEvaluatorBool(const QString &instrName, const QString &attrName, const QString &cond);
    DataModel::EvaluatorId createEvaluatorVariant(const QString &instrName, const QString &attrName, const QString &cond);

    virtual QString createContextString(const QString &instrName) const = 0;
    virtual QString createContext(const QString &instrName, const QString &attrName, const QString &attrValue) const = 0;

    const DataModel::EvaluatorInfos &evaluators() const { return m_evaluators; }
    const DataModel::AssignmentInfos &assignments() const { return m_assignments; }
    const DataModel::ForeachInfos &foreaches() const { return m_foreaches; }
    const QVector<ExecutableContent::StringId> &dataIds() const { return m_dataIds; }

private:
    template <typename T, typename U>
    class Table {
        QVector<T> elements;
        QMap<T, int> indexForElement;

    public:
        U add(const T &s) {
            int pos = indexForElement.value(s, -1);
            if (pos == -1) {
                pos = elements.size();
                elements.append(s);
                indexForElement.insert(s, pos);
            }
            return pos;
        }

        QVector<T> data() {
            elements.squeeze();
            return elements;
        }
    };
    Table<QString, StringId> m_stringTable;
    Table<QByteArray, ByteArrayId> m_byteArrayTable;

    struct SequenceInfo {
        int location;
        qint32 entryCount; // the amount of qint32's that the instructions take up
    };
    QVector<SequenceInfo> m_activeSequences;

    class {
    public:
        ContainerId newContainerId() const { return m_instr.size(); }

        template <typename T>
        T *add(int extra = 0)
        {
            int pos = m_instr.size();
            int size = sizeof(T) / sizeof(qint32) + extra;
            if (m_info)
                m_info->entryCount += size;
            m_instr.resize(pos + size);
            T *instr = at<T>(pos);
            Q_ASSERT(instr->instructionType == 0);
            instr->instructionType = T::kind();
//            qDebug()<<"adding instruction"<<typeid(*instr).name()<<"size"<<size;
            return instr;
        }

        int offset(Instruction *instr) const
        {
            return reinterpret_cast<qint32 *>(instr) - m_instr.data();
        }

        template <typename T>
        T *at(int offset)
        {
            return reinterpret_cast<T *>(&m_instr[offset]);
        }

        QVector<qint32> data()
        {
            m_instr.squeeze();
            return m_instr;
        }

        void setSequenceInfo(SequenceInfo *info)
        {
            m_info = info;
        }

    private:
        QVector<qint32> m_instr;
        SequenceInfo *m_info = nullptr;
    } m_instructions;


    DataModel::EvaluatorId addEvaluator(const QString &expr, const QString &context)
    {
        DataModel::EvaluatorInfo ei;
        ei.expr = m_stringTable.add(expr);
        ei.context = m_stringTable.add(context);
        m_evaluators.append(ei);
        return m_evaluators.size() - 1;
    }

    DataModel::EvaluatorId addAssignment(const QString &dest, const QString &expr, const QString &context)
    {
        DataModel::AssignmentInfo ai;
        ai.dest = m_stringTable.add(dest);
        ai.expr = m_stringTable.add(expr);
        ai.context = m_stringTable.add(context);
        m_assignments.append(ai);
        return m_assignments.size() - 1;
    }

    DataModel::EvaluatorId addForeach(const QString &array, const QString &item, const QString &index, const QString &context)
    {
        DataModel::ForeachInfo fi;
        fi.array = m_stringTable.add(array);
        fi.item = m_stringTable.add(item);
        fi.index = m_stringTable.add(index);
        fi.context = m_stringTable.add(context);
        m_foreaches.append(fi);
        return m_foreaches.size() - 1;
    }

    DataModel::EvaluatorId addDataElement(const QString &id, const QString &expr, const QString &context)
    {
        auto str = m_stringTable.add(id);
        if (!m_dataIds.contains(str))
            m_dataIds.append(str);
        if (expr.isEmpty())
            return DataModel::NoEvaluator;

        return addAssignment(id, expr, context);
    }

    DataModel::EvaluatorInfos m_evaluators;
    DataModel::AssignmentInfos m_assignments;
    DataModel::ForeachInfos m_foreaches;
    QVector<ExecutableContent::StringId> m_dataIds;
};

} // ExecutableContent namespace
} // namespace Scxml

#endif // EXECUTABLECONTENT_P_H
