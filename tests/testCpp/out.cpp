#include "out.h"

#include <QScxml/nulldatamodel.h>
#include <QScxml/executablecontent.h>

struct StateMachine::Data: private Scxml::TableData {
    Data(StateMachine &table)
        : table(table)
        , state_s__1(&table)
        , transition_s__1_0(&state_s__1, { byteArray(0) })
        , transition_s__1_1(&state_s__1, { byteArray(1) })
        , transition_s__1_2(&state_s__1, { byteArray(2) })
        , transition_s__1_3(&state_s__1, { byteArray(3) })
        , transition_s__1_4(&state_s__1, { byteArray(4) })
        , state_s__2(&table)
        , state_s__2__1(&state_s__2)
        , state_s__2__1__1(&state_s__2__1)
        , transition_s__2__1__1_0(&state_s__2__1__1, { byteArray(5) })
        , transition_s__2__1__1_1(&state_s__2__1__1, { byteArray(6) })
        , transition_s__2__1__1_2(&state_s__2__1__1, { byteArray(7) })
        , transition_s__2__1__1_3(&state_s__2__1__1, { byteArray(8) })
        , state_s__2__1__2(&state_s__2__1)
        , state_s__2__1__2__1(&state_s__2__1__2)
        , state_s__2__1__2__1__1(&state_s__2__1__2__1)
        , state_s__2__1__2__1__1__1(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__1_0(&state_s__2__1__2__1__1__1, { byteArray(9) })
        , transition_s__2__1__2__1__1__1_1(&state_s__2__1__2__1__1__1, { byteArray(10) })
        , transition_s__2__1__2__1__1__1_2(&state_s__2__1__2__1__1__1, { byteArray(11) })
        , transition_s__2__1__2__1__1__1_3(&state_s__2__1__2__1__1__1, { byteArray(12) })
        , transition_s__2__1__2__1__1__1_4(&state_s__2__1__2__1__1__1, { byteArray(2) })
        , state_s__2__1__2__1__1__2(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__2_0(&state_s__2__1__2__1__1__2, { byteArray(13) })
        , transition_s__2__1__2__1__1__2_1(&state_s__2__1__2__1__1__2, { byteArray(12) })
        , transition_s__2__1__2__1__1__2_2(&state_s__2__1__2__1__1__2, { byteArray(14) })
        , transition_s__2__1__2__1__1__2_3(&state_s__2__1__2__1__1__2, { byteArray(15) })
        , state_s__2__1__2__1__1__3(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__3_0(&state_s__2__1__2__1__1__3, { byteArray(16) })
        , transition_s__2__1__2__1__1__3_1(&state_s__2__1__2__1__1__3, { byteArray(17) })
        , transition_s__2__1__2__1__1__3_2(&state_s__2__1__2__1__1__3, { byteArray(18) })
        , transition_s__2__1__2__1__1__3_3(&state_s__2__1__2__1__1__3, { byteArray(19) })
        , transition_s__2__1__2__1__1__3_4(&state_s__2__1__2__1__1__3, { byteArray(20) })
        , transition_s__2__1__2__1__1__3_5(&state_s__2__1__2__1__1__3, { byteArray(21) })
        , transition_s__2__1__2__1__1__3_6(&state_s__2__1__2__1__1__3, { byteArray(22) })
        , transition_s__2__1__2__1__1_3(&state_s__2__1__2__1__1, { byteArray(23) })
        , transition_s__2__1__2__1__1_4(&state_s__2__1__2__1__1, { byteArray(24) })
        , transition_s__2__1__2__1__1_5(&state_s__2__1__2__1__1, { byteArray(20) })
        , transition_s__2__1__2__1__1_6(&state_s__2__1__2__1__1, { byteArray(25) })
        , transition_s__2__1__2__1__1_7(&state_s__2__1__2__1__1, { byteArray(26) })
        , transition_s__2__1__2__1_1(&state_s__2__1__2__1, { byteArray(27) })
        , transition_s__2__1__2__1_2(&state_s__2__1__2__1, { byteArray(0) })
        , transition_s__2__1__2__1_3(&state_s__2__1__2__1, { byteArray(6) })
        , transition_s__2__1__2__1_4(&state_s__2__1__2__1, { byteArray(28) })
        , transition_s__2__1__2_1(&state_s__2__1__2, { byteArray(17) })
        , transition_s__2__1__2_2(&state_s__2__1__2, { byteArray(29) })
        , transition_s__2__1__2_3(&state_s__2__1__2, { byteArray(6) })
        , transition_s__2__1__2_4(&state_s__2__1__2, { byteArray(1) })
        , transition_s__2__1__2_5(&state_s__2__1__2, { byteArray(3) })
        , transition_s__2__1__2_6(&state_s__2__1__2, { byteArray(30) })
        , transition_s__2__1_2(&state_s__2__1, { byteArray(31) })
        , transition_s__2__1_3(&state_s__2__1, { byteArray(32) })
        , transition_s__2__1_4(&state_s__2__1, { byteArray(33) })
        , transition_s__2__1_5(&state_s__2__1, { byteArray(34) })
        , transition_s__2__1_6(&state_s__2__1, { byteArray(35) })
        , transition_s__2_1(&state_s__2, { byteArray(36) })
        , transition_s__2_2(&state_s__2, { byteArray(25) })
        , transition_s__2_3(&state_s__2, { byteArray(13) })
    { init(); }

    void init() {
        table.setDataModel(new Scxml::NullDataModel(&table));
        table.setDataBinding(Scxml::StateTable::EarlyBinding);
        table.setTableData(this);
        table.setInitialState(&state_s__1);
        state_s__1.setObjectName(string(0));
        state_s__1.addTransition(&transition_s__1_0);
        transition_s__1_0.setTargetStates({ &state_s__1 });
        state_s__1.addTransition(&transition_s__1_1);
        transition_s__1_1.setTargetStates({ &state_s__1 });
        state_s__1.addTransition(&transition_s__1_2);
        transition_s__1_2.setTargetStates({ &state_s__1 });
        state_s__1.addTransition(&transition_s__1_3);
        transition_s__1_3.setTargetStates({ &state_s__1 });
        state_s__1.addTransition(&transition_s__1_4);
        transition_s__1_4.setTargetStates({ &state_s__1 });
        state_s__2.setObjectName(string(1));
        state_s__2.setInitialState(&state_s__2__1);
        state_s__2__1.setObjectName(string(2));
        state_s__2__1.setInitialState(&state_s__2__1__1);
        state_s__2__1__1.setObjectName(string(3));
        state_s__2__1__1.addTransition(&transition_s__2__1__1_0);
        transition_s__2__1__1_0.setTargetStates({ &state_s__2 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_1);
        transition_s__2__1__1_1.setTargetStates({ &state_s__2 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_2);
        transition_s__2__1__1_2.setTargetStates({ &state_s__2__1 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_3);
        transition_s__2__1__1_3.setTargetStates({ &state_s__2 });
        state_s__2__1__2.setObjectName(string(4));
        state_s__2__1__2.setInitialState(&state_s__2__1__2__1);
        state_s__2__1__2__1.setObjectName(string(5));
        state_s__2__1__2__1.setInitialState(&state_s__2__1__2__1__1);
        state_s__2__1__2__1__1.setObjectName(string(6));
        state_s__2__1__2__1__1.setInitialState(&state_s__2__1__2__1__1__1);
        state_s__2__1__2__1__1__1.setObjectName(string(7));
        state_s__2__1__2__1__1__1.addTransition(&transition_s__2__1__2__1__1__1_0);
        transition_s__2__1__2__1__1__1_0.setTargetStates({ &state_s__2__1__1 });
        state_s__2__1__2__1__1__1.addTransition(&transition_s__2__1__2__1__1__1_1);
        transition_s__2__1__2__1__1__1_1.setTargetStates({ &state_s__2__1__2__1__1__1 });
        state_s__2__1__2__1__1__1.addTransition(&transition_s__2__1__2__1__1__1_2);
        transition_s__2__1__2__1__1__1_2.setTargetStates({ &state_s__2__1__2 });
        state_s__2__1__2__1__1__1.addTransition(&transition_s__2__1__2__1__1__1_3);
        transition_s__2__1__2__1__1__1_3.setTargetStates({ &state_s__2__1 });
        state_s__2__1__2__1__1__1.addTransition(&transition_s__2__1__2__1__1__1_4);
        transition_s__2__1__2__1__1__1_4.setTargetStates({ &state_s__1 });
        state_s__2__1__2__1__1__2.setObjectName(string(8));
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_0);
        transition_s__2__1__2__1__1__2_0.setTargetStates({ &state_s__2__1__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_1);
        transition_s__2__1__2__1__1__2_1.setTargetStates({ &state_s__2__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_2);
        transition_s__2__1__2__1__1__2_2.setTargetStates({ &state_s__2__1__2__1__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_3);
        transition_s__2__1__2__1__1__2_3.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1__2__1__1__3.setObjectName(string(9));
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_0);
        transition_s__2__1__2__1__1__3_0.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_1);
        transition_s__2__1__2__1__1__3_1.setTargetStates({ &state_s__2__1__2 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_2);
        transition_s__2__1__2__1__1__3_2.setTargetStates({ &state_s__2__1__2__1__1__3 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_3);
        transition_s__2__1__2__1__1__3_3.setTargetStates({ &state_s__2__1__1 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_4);
        transition_s__2__1__2__1__1__3_4.setTargetStates({ &state_s__1 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_5);
        transition_s__2__1__2__1__1__3_5.setTargetStates({ &state_s__2 });
        state_s__2__1__2__1__1__3.addTransition(&transition_s__2__1__2__1__1__3_6);
        transition_s__2__1__2__1__1__3_6.setTargetStates({ &state_s__2__1__2__1__1__2 });
        state_s__2__1__2__1__1.addTransition(&transition_s__2__1__2__1__1_3);
        transition_s__2__1__2__1__1_3.setTargetStates({ &state_s__2__1__2 });
        state_s__2__1__2__1__1.addTransition(&transition_s__2__1__2__1__1_4);
        transition_s__2__1__2__1__1_4.setTargetStates({ &state_s__2__1__2__1__1 });
        state_s__2__1__2__1__1.addTransition(&transition_s__2__1__2__1__1_5);
        transition_s__2__1__2__1__1_5.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1__2__1__1.addTransition(&transition_s__2__1__2__1__1_6);
        transition_s__2__1__2__1__1_6.setTargetStates({ &state_s__2__1__2 });
        state_s__2__1__2__1__1.addTransition(&transition_s__2__1__2__1__1_7);
        transition_s__2__1__2__1__1_7.setTargetStates({ &state_s__2__1__1 });
        state_s__2__1__2__1.addTransition(&transition_s__2__1__2__1_1);
        transition_s__2__1__2__1_1.setTargetStates({ &state_s__1 });
        state_s__2__1__2__1.addTransition(&transition_s__2__1__2__1_2);
        transition_s__2__1__2__1_2.setTargetStates({ &state_s__2 });
        state_s__2__1__2__1.addTransition(&transition_s__2__1__2__1_3);
        transition_s__2__1__2__1_3.setTargetStates({ &state_s__2__1 });
        state_s__2__1__2__1.addTransition(&transition_s__2__1__2__1_4);
        transition_s__2__1__2__1_4.setTargetStates({ &state_s__2 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_1);
        transition_s__2__1__2_1.setTargetStates({ &state_s__2__1__2__1__1 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_2);
        transition_s__2__1__2_2.setTargetStates({ &state_s__2__1__2__1__1 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_3);
        transition_s__2__1__2_3.setTargetStates({ &state_s__2__1 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_4);
        transition_s__2__1__2_4.setTargetStates({ &state_s__2__1__2__1__1__2 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_5);
        transition_s__2__1__2_5.setTargetStates({ &state_s__2 });
        state_s__2__1__2.addTransition(&transition_s__2__1__2_6);
        transition_s__2__1__2_6.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1.addTransition(&transition_s__2__1_2);
        transition_s__2__1_2.setTargetStates({ &state_s__2__1 });
        state_s__2__1.addTransition(&transition_s__2__1_3);
        transition_s__2__1_3.setTargetStates({ &state_s__2__1__2__1__1__1 });
        state_s__2__1.addTransition(&transition_s__2__1_4);
        transition_s__2__1_4.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1.addTransition(&transition_s__2__1_5);
        transition_s__2__1_5.setTargetStates({ &state_s__2__1 });
        state_s__2__1.addTransition(&transition_s__2__1_6);
        transition_s__2__1_6.setTargetStates({ &state_s__2__1__1 });
        state_s__2.addTransition(&transition_s__2_1);
        transition_s__2_1.setTargetStates({ &state_s__2 });
        state_s__2.addTransition(&transition_s__2_2);
        transition_s__2_2.setTargetStates({ &state_s__2__1__2 });
        state_s__2.addTransition(&transition_s__2_3);
        transition_s__2_3.setTargetStates({ &state_s__2__1__2__1 });
    }

    Scxml::ExecutableContent::Instructions instructions() const Q_DECL_OVERRIDE
    { return theInstructions; }
    
    QString string(Scxml::ExecutableContent::StringId id) const Q_DECL_OVERRIDE Q_DECL_FINAL
    {
        Q_ASSERT(id >= Scxml::ExecutableContent::NoString); Q_ASSERT(id < 10);
        if (id == Scxml::ExecutableContent::NoString) return QString();
        return QString({static_cast<QStringData*>(strings.data + id)});
    }
    
    QByteArray byteArray(Scxml::ExecutableContent::ByteArrayId id) const Q_DECL_OVERRIDE Q_DECL_FINAL
    {
        Q_ASSERT(id >= Scxml::ExecutableContent::NoString); Q_ASSERT(id < 37);
        if (id == Scxml::ExecutableContent::NoString) return QByteArray();
        return QByteArray({byteArrays.data + id});
    }
    
    Scxml::ExecutableContent::StringId *dataNames(int *count) const Q_DECL_OVERRIDE
    { *count = 0; return dataIds; }
    
    Scxml::EvaluatorInfo evaluatorInfo(Scxml::EvaluatorId evaluatorId) const Q_DECL_OVERRIDE
    { Q_ASSERT(evaluatorId >= 0); Q_ASSERT(evaluatorId < 0); return evaluators[evaluatorId]; }
    
    Scxml::AssignmentInfo assignmentInfo(Scxml::EvaluatorId assignmentId) const Q_DECL_OVERRIDE
    { Q_ASSERT(assignmentId >= 0); Q_ASSERT(assignmentId < 0); return assignments[assignmentId]; }
    
    Scxml::ForeachInfo foreachInfo(Scxml::EvaluatorId foreachId) const Q_DECL_OVERRIDE
    { Q_ASSERT(foreachId >= 0); Q_ASSERT(foreachId < 0); return foreaches[foreachId]; }

    StateMachine &table;
    Scxml::ScxmlState state_s__1;
    Scxml::ScxmlTransition transition_s__1_0;
    Scxml::ScxmlTransition transition_s__1_1;
    Scxml::ScxmlTransition transition_s__1_2;
    Scxml::ScxmlTransition transition_s__1_3;
    Scxml::ScxmlTransition transition_s__1_4;
    Scxml::ScxmlState state_s__2;
    Scxml::ScxmlState state_s__2__1;
    Scxml::ScxmlState state_s__2__1__1;
    Scxml::ScxmlTransition transition_s__2__1__1_0;
    Scxml::ScxmlTransition transition_s__2__1__1_1;
    Scxml::ScxmlTransition transition_s__2__1__1_2;
    Scxml::ScxmlTransition transition_s__2__1__1_3;
    Scxml::ScxmlState state_s__2__1__2;
    Scxml::ScxmlState state_s__2__1__2__1;
    Scxml::ScxmlState state_s__2__1__2__1__1;
    Scxml::ScxmlState state_s__2__1__2__1__1__1;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__1_0;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__1_1;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__1_2;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__1_3;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__1_4;
    Scxml::ScxmlState state_s__2__1__2__1__1__2;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__2_0;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__2_1;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__2_2;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__2_3;
    Scxml::ScxmlState state_s__2__1__2__1__1__3;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_0;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_1;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_2;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_3;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_4;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_5;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1__3_6;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1_3;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1_4;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1_5;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1_6;
    Scxml::ScxmlTransition transition_s__2__1__2__1__1_7;
    Scxml::ScxmlTransition transition_s__2__1__2__1_1;
    Scxml::ScxmlTransition transition_s__2__1__2__1_2;
    Scxml::ScxmlTransition transition_s__2__1__2__1_3;
    Scxml::ScxmlTransition transition_s__2__1__2__1_4;
    Scxml::ScxmlTransition transition_s__2__1__2_1;
    Scxml::ScxmlTransition transition_s__2__1__2_2;
    Scxml::ScxmlTransition transition_s__2__1__2_3;
    Scxml::ScxmlTransition transition_s__2__1__2_4;
    Scxml::ScxmlTransition transition_s__2__1__2_5;
    Scxml::ScxmlTransition transition_s__2__1__2_6;
    Scxml::ScxmlTransition transition_s__2__1_2;
    Scxml::ScxmlTransition transition_s__2__1_3;
    Scxml::ScxmlTransition transition_s__2__1_4;
    Scxml::ScxmlTransition transition_s__2__1_5;
    Scxml::ScxmlTransition transition_s__2__1_6;
    Scxml::ScxmlTransition transition_s__2_1;
    Scxml::ScxmlTransition transition_s__2_2;
    Scxml::ScxmlTransition transition_s__2_3;
    
    static qint32 theInstructions[];
    static struct Strings {
        QArrayData data[10];
        qunicodechar stringdata[95];
    } strings;
    static struct ByteArrays {
        QByteArrayData data[37];
        char stringdata[149];
    } byteArrays;
    static Scxml::ExecutableContent::StringId dataIds [];
    static Scxml::EvaluatorInfo evaluators[];
    static Scxml::AssignmentInfo assignments[];
    static Scxml::ForeachInfo foreaches[];
};

StateMachine::StateMachine(QObject *parent)
    : Scxml::StateTable(parent)
    , data(new Data(*this))
{}

StateMachine::~StateMachine()
{ delete data; }


void StateMachine::event_E10()
{ submitEvent(data->byteArray(18)); }

void StateMachine::event_E100()
{ submitEvent(data->byteArray(32)); }

void StateMachine::event_E13()
{ submitEvent(data->byteArray(22)); }

void StateMachine::event_E16()
{ submitEvent(data->byteArray(31)); }

void StateMachine::event_E17()
{ submitEvent(data->byteArray(4)); }

void StateMachine::event_E19()
{ submitEvent(data->byteArray(0)); }

void StateMachine::event_E21()
{ submitEvent(data->byteArray(16)); }

void StateMachine::event_E22()
{ submitEvent(data->byteArray(6)); }

void StateMachine::event_E25()
{ submitEvent(data->byteArray(17)); }

void StateMachine::event_E28()
{ submitEvent(data->byteArray(13)); }

void StateMachine::event_E29()
{ submitEvent(data->byteArray(24)); }

void StateMachine::event_E33()
{ submitEvent(data->byteArray(35)); }

void StateMachine::event_E37()
{ submitEvent(data->byteArray(2)); }

void StateMachine::event_E38()
{ submitEvent(data->byteArray(14)); }

void StateMachine::event_E47()
{ submitEvent(data->byteArray(1)); }

void StateMachine::event_E49()
{ submitEvent(data->byteArray(25)); }

void StateMachine::event_E52()
{ submitEvent(data->byteArray(34)); }

void StateMachine::event_E54()
{ submitEvent(data->byteArray(15)); }

void StateMachine::event_E55()
{ submitEvent(data->byteArray(30)); }

void StateMachine::event_E56()
{ submitEvent(data->byteArray(7)); }

void StateMachine::event_E58()
{ submitEvent(data->byteArray(26)); }

void StateMachine::event_E65()
{ submitEvent(data->byteArray(28)); }

void StateMachine::event_E68()
{ submitEvent(data->byteArray(8)); }

void StateMachine::event_E70()
{ submitEvent(data->byteArray(33)); }

void StateMachine::event_E75()
{ submitEvent(data->byteArray(10)); }

void StateMachine::event_E77()
{ submitEvent(data->byteArray(9)); }

void StateMachine::event_E8()
{ submitEvent(data->byteArray(12)); }

void StateMachine::event_E80()
{ submitEvent(data->byteArray(27)); }

void StateMachine::event_E82()
{ submitEvent(data->byteArray(19)); }

void StateMachine::event_E83()
{ submitEvent(data->byteArray(11)); }

void StateMachine::event_E85()
{ submitEvent(data->byteArray(36)); }

void StateMachine::event_E86()
{ submitEvent(data->byteArray(21)); }

void StateMachine::event_E88()
{ submitEvent(data->byteArray(23)); }

void StateMachine::event_E89()
{ submitEvent(data->byteArray(5)); }

void StateMachine::event_E93()
{ submitEvent(data->byteArray(3)); }

void StateMachine::event_E97()
{ submitEvent(data->byteArray(29)); }

void StateMachine::event_E99()
{ submitEvent(data->byteArray(20)); }

qint32 StateMachine::Data::theInstructions[] = {
};

#define STR_LIT(idx, ofs, len) \
    Q_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(Strings, stringdata) + ofs * sizeof(qunicodechar) - idx * sizeof(QArrayData)) \
    )
StateMachine::Data::Strings StateMachine::Data::strings = {{
STR_LIT(0, 0, 3), STR_LIT(1, 4, 3), STR_LIT(2, 8, 5), STR_LIT(3, 14, 7),
STR_LIT(4, 22, 7), STR_LIT(5, 30, 9), STR_LIT(6, 40, 11), STR_LIT(7, 52, 13),
STR_LIT(8, 66, 13), STR_LIT(9, 80, 13)
},
QT_UNICODE_LITERAL_II("\x73\x5f\x31\x00") // s_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x00") // s_2
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x00") // s_2_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x31\x00") // s_2_1_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x00") // s_2_1_2
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x5f\x31\x00") // s_2_1_2_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x5f\x31\x5f\x31\x00") // s_2_1_2_1_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x5f\x31\x5f\x31\x5f\x31\x00") // s_2_1_2_1_1_1
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x5f\x31\x5f\x31\x5f\x32\x00") // s_2_1_2_1_1_2
QT_UNICODE_LITERAL_II("\x73\x5f\x32\x5f\x31\x5f\x32\x5f\x31\x5f\x31\x5f\x33\x00") // s_2_1_2_1_1_3
};

#define BA_LIT(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(ByteArrays, stringdata) + ofs - idx * sizeof(QByteArrayData)) \
    )
StateMachine::Data::ByteArrays StateMachine::Data::byteArrays = {{
BA_LIT(0, 0, 3), BA_LIT(1, 4, 3), BA_LIT(2, 8, 3), BA_LIT(3, 12, 3),
BA_LIT(4, 16, 3), BA_LIT(5, 20, 3), BA_LIT(6, 24, 3), BA_LIT(7, 28, 3),
BA_LIT(8, 32, 3), BA_LIT(9, 36, 3), BA_LIT(10, 40, 3), BA_LIT(11, 44, 3),
BA_LIT(12, 48, 2), BA_LIT(13, 51, 3), BA_LIT(14, 55, 3), BA_LIT(15, 59, 3),
BA_LIT(16, 63, 3), BA_LIT(17, 67, 3), BA_LIT(18, 71, 3), BA_LIT(19, 75, 3),
BA_LIT(20, 79, 3), BA_LIT(21, 83, 3), BA_LIT(22, 87, 3), BA_LIT(23, 91, 3),
BA_LIT(24, 95, 3), BA_LIT(25, 99, 3), BA_LIT(26, 103, 3), BA_LIT(27, 107, 3),
BA_LIT(28, 111, 3), BA_LIT(29, 115, 3), BA_LIT(30, 119, 3), BA_LIT(31, 123, 3),
BA_LIT(32, 127, 4), BA_LIT(33, 132, 3), BA_LIT(34, 136, 3), BA_LIT(35, 140, 3),
BA_LIT(36, 144, 3)
},
"\x45\x31\x39\x00" // E19
"\x45\x34\x37\x00" // E47
"\x45\x33\x37\x00" // E37
"\x45\x39\x33\x00" // E93
"\x45\x31\x37\x00" // E17
"\x45\x38\x39\x00" // E89
"\x45\x32\x32\x00" // E22
"\x45\x35\x36\x00" // E56
"\x45\x36\x38\x00" // E68
"\x45\x37\x37\x00" // E77
"\x45\x37\x35\x00" // E75
"\x45\x38\x33\x00" // E83
"\x45\x38\x00" // E8
"\x45\x32\x38\x00" // E28
"\x45\x33\x38\x00" // E38
"\x45\x35\x34\x00" // E54
"\x45\x32\x31\x00" // E21
"\x45\x32\x35\x00" // E25
"\x45\x31\x30\x00" // E10
"\x45\x38\x32\x00" // E82
"\x45\x39\x39\x00" // E99
"\x45\x38\x36\x00" // E86
"\x45\x31\x33\x00" // E13
"\x45\x38\x38\x00" // E88
"\x45\x32\x39\x00" // E29
"\x45\x34\x39\x00" // E49
"\x45\x35\x38\x00" // E58
"\x45\x38\x30\x00" // E80
"\x45\x36\x35\x00" // E65
"\x45\x39\x37\x00" // E97
"\x45\x35\x35\x00" // E55
"\x45\x31\x36\x00" // E16
"\x45\x31\x30\x30\x00" // E100
"\x45\x37\x30\x00" // E70
"\x45\x35\x32\x00" // E52
"\x45\x33\x33\x00" // E33
"\x45\x38\x35\x00" // E85
};

Scxml::ExecutableContent::StringId StateMachine::Data::dataIds[] = {
};

Scxml::EvaluatorInfo StateMachine::Data::evaluators[] = {
};

Scxml::AssignmentInfo StateMachine::Data::assignments[] = {
};

Scxml::ForeachInfo StateMachine::Data::foreaches[] = {
};

