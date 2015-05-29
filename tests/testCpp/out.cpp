#include "out.h"

#include <QScxmlLib/nulldatamodel.h>
#include <QScxmlLib/executablecontent.h>

struct StateMachine::Data: private Scxml::TableData {
    Data(StateMachine &table)
        : table(table)
        , state_s__1(&table)
        , transition_s__1_0(&state_s__1, { QByteArray::fromRawData("E19", 3) })
        , transition_s__1_1(&state_s__1, { QByteArray::fromRawData("E47", 3) })
        , transition_s__1_2(&state_s__1, { QByteArray::fromRawData("E37", 3) })
        , transition_s__1_3(&state_s__1, { QByteArray::fromRawData("E93", 3) })
        , transition_s__1_4(&state_s__1, { QByteArray::fromRawData("E17", 3) })
        , state_s__2(&table)
        , state_s__2__1(&state_s__2)
        , state_s__2__1__1(&state_s__2__1)
        , transition_s__2__1__1_0(&state_s__2__1__1, { QByteArray::fromRawData("E89", 3) })
        , transition_s__2__1__1_1(&state_s__2__1__1, { QByteArray::fromRawData("E22", 3) })
        , transition_s__2__1__1_2(&state_s__2__1__1, { QByteArray::fromRawData("E56", 3) })
        , transition_s__2__1__1_3(&state_s__2__1__1, { QByteArray::fromRawData("E68", 3) })
        , state_s__2__1__2(&state_s__2__1)
        , state_s__2__1__2__1(&state_s__2__1__2)
        , state_s__2__1__2__1__1(&state_s__2__1__2__1)
        , state_s__2__1__2__1__1__1(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__1_0(&state_s__2__1__2__1__1__1, { QByteArray::fromRawData("E77", 3) })
        , transition_s__2__1__2__1__1__1_1(&state_s__2__1__2__1__1__1, { QByteArray::fromRawData("E75", 3) })
        , transition_s__2__1__2__1__1__1_2(&state_s__2__1__2__1__1__1, { QByteArray::fromRawData("E83", 3) })
        , transition_s__2__1__2__1__1__1_3(&state_s__2__1__2__1__1__1, { QByteArray::fromRawData("E8", 2) })
        , transition_s__2__1__2__1__1__1_4(&state_s__2__1__2__1__1__1, { QByteArray::fromRawData("E37", 3) })
        , state_s__2__1__2__1__1__2(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__2_0(&state_s__2__1__2__1__1__2, { QByteArray::fromRawData("E28", 3) })
        , transition_s__2__1__2__1__1__2_1(&state_s__2__1__2__1__1__2, { QByteArray::fromRawData("E8", 2) })
        , transition_s__2__1__2__1__1__2_2(&state_s__2__1__2__1__1__2, { QByteArray::fromRawData("E38", 3) })
        , transition_s__2__1__2__1__1__2_3(&state_s__2__1__2__1__1__2, { QByteArray::fromRawData("E54", 3) })
        , state_s__2__1__2__1__1__3(&state_s__2__1__2__1__1)
        , transition_s__2__1__2__1__1__3_0(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E21", 3) })
        , transition_s__2__1__2__1__1__3_1(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E25", 3) })
        , transition_s__2__1__2__1__1__3_2(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E10", 3) })
        , transition_s__2__1__2__1__1__3_3(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E82", 3) })
        , transition_s__2__1__2__1__1__3_4(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E99", 3) })
        , transition_s__2__1__2__1__1__3_5(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E86", 3) })
        , transition_s__2__1__2__1__1__3_6(&state_s__2__1__2__1__1__3, { QByteArray::fromRawData("E13", 3) })
        , transition_s__2__1__2__1__1_3(&state_s__2__1__2__1__1, { QByteArray::fromRawData("E88", 3) })
        , transition_s__2__1__2__1__1_4(&state_s__2__1__2__1__1, { QByteArray::fromRawData("E29", 3) })
        , transition_s__2__1__2__1__1_5(&state_s__2__1__2__1__1, { QByteArray::fromRawData("E99", 3) })
        , transition_s__2__1__2__1__1_6(&state_s__2__1__2__1__1, { QByteArray::fromRawData("E49", 3) })
        , transition_s__2__1__2__1__1_7(&state_s__2__1__2__1__1, { QByteArray::fromRawData("E58", 3) })
        , transition_s__2__1__2__1_1(&state_s__2__1__2__1, { QByteArray::fromRawData("E80", 3) })
        , transition_s__2__1__2__1_2(&state_s__2__1__2__1, { QByteArray::fromRawData("E19", 3) })
        , transition_s__2__1__2__1_3(&state_s__2__1__2__1, { QByteArray::fromRawData("E22", 3) })
        , transition_s__2__1__2__1_4(&state_s__2__1__2__1, { QByteArray::fromRawData("E65", 3) })
        , transition_s__2__1__2_1(&state_s__2__1__2, { QByteArray::fromRawData("E25", 3) })
        , transition_s__2__1__2_2(&state_s__2__1__2, { QByteArray::fromRawData("E97", 3) })
        , transition_s__2__1__2_3(&state_s__2__1__2, { QByteArray::fromRawData("E22", 3) })
        , transition_s__2__1__2_4(&state_s__2__1__2, { QByteArray::fromRawData("E47", 3) })
        , transition_s__2__1__2_5(&state_s__2__1__2, { QByteArray::fromRawData("E93", 3) })
        , transition_s__2__1__2_6(&state_s__2__1__2, { QByteArray::fromRawData("E55", 3) })
        , transition_s__2__1_2(&state_s__2__1, { QByteArray::fromRawData("E16", 3) })
        , transition_s__2__1_3(&state_s__2__1, { QByteArray::fromRawData("E100", 4) })
        , transition_s__2__1_4(&state_s__2__1, { QByteArray::fromRawData("E70", 3) })
        , transition_s__2__1_5(&state_s__2__1, { QByteArray::fromRawData("E52", 3) })
        , transition_s__2__1_6(&state_s__2__1, { QByteArray::fromRawData("E33", 3) })
        , transition_s__2_1(&state_s__2, { QByteArray::fromRawData("E85", 3) })
        , transition_s__2_2(&state_s__2, { QByteArray::fromRawData("E49", 3) })
        , transition_s__2_3(&state_s__2, { QByteArray::fromRawData("E28", 3) })
    {}

    void init() {
        table.setDataModel(new Scxml::NullDataModel(&table));
        table.dataModel()->setEvaluators(evaluators, assignments, foreaches);
        table.setDataItemNames(dataIds);
        table.setDataBinding(Scxml::StateTable::EarlyBinding);
        table.setTableData(this);
        table.setInitialState(&state_s__1);
        state_s__1.setObjectName(QStringLiteral("s_1"));
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
        state_s__2.setObjectName(QStringLiteral("s_2"));
        state_s__2.setInitialState(&state_s__2__1);
        state_s__2__1.setObjectName(QStringLiteral("s_2_1"));
        state_s__2__1.setInitialState(&state_s__2__1__1);
        state_s__2__1__1.setObjectName(QStringLiteral("s_2_1_1"));
        state_s__2__1__1.addTransition(&transition_s__2__1__1_0);
        transition_s__2__1__1_0.setTargetStates({ &state_s__2 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_1);
        transition_s__2__1__1_1.setTargetStates({ &state_s__2 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_2);
        transition_s__2__1__1_2.setTargetStates({ &state_s__2__1 });
        state_s__2__1__1.addTransition(&transition_s__2__1__1_3);
        transition_s__2__1__1_3.setTargetStates({ &state_s__2 });
        state_s__2__1__2.setObjectName(QStringLiteral("s_2_1_2"));
        state_s__2__1__2.setInitialState(&state_s__2__1__2__1);
        state_s__2__1__2__1.setObjectName(QStringLiteral("s_2_1_2_1"));
        state_s__2__1__2__1.setInitialState(&state_s__2__1__2__1__1);
        state_s__2__1__2__1__1.setObjectName(QStringLiteral("s_2_1_2_1_1"));
        state_s__2__1__2__1__1.setInitialState(&state_s__2__1__2__1__1__1);
        state_s__2__1__2__1__1__1.setObjectName(QStringLiteral("s_2_1_2_1_1_1"));
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
        state_s__2__1__2__1__1__2.setObjectName(QStringLiteral("s_2_1_2_1_1_2"));
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_0);
        transition_s__2__1__2__1__1__2_0.setTargetStates({ &state_s__2__1__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_1);
        transition_s__2__1__2__1__1__2_1.setTargetStates({ &state_s__2__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_2);
        transition_s__2__1__2__1__1__2_2.setTargetStates({ &state_s__2__1__2__1__1 });
        state_s__2__1__2__1__1__2.addTransition(&transition_s__2__1__2__1__1__2_3);
        transition_s__2__1__2__1__1__2_3.setTargetStates({ &state_s__2__1__2__1 });
        state_s__2__1__2__1__1__3.setObjectName(QStringLiteral("s_2_1_2_1_1_3"));
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

    QString string(Scxml::ExecutableContent::StringId id) const Q_DECL_OVERRIDE
    { return id == Scxml::ExecutableContent::NoString ? QString() : strings.at(id); }

    QByteArray byteArray(Scxml::ExecutableContent::ByteArrayId id) const Q_DECL_OVERRIDE
    { return byteArrays.at(id); }

    Scxml::ExecutableContent::Instructions instructions() const Q_DECL_OVERRIDE
    { return theInstructions; }

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
    static QVector<QString> strings;
    static QVector<QByteArray> byteArrays;
    static Scxml::ExecutableContent::StringIds dataIds;
    static Scxml::EvaluatorInfos evaluators;
    static Scxml::AssignmentInfos assignments;
    static Scxml::ForeachInfos foreaches;
};

StateMachine::StateMachine(QObject *parent)
    : Scxml::StateTable(parent)
    , data(new Data(*this))
{}

StateMachine::~StateMachine()
{ delete data; }

bool StateMachine::init()
{ data->init(); return StateTable::init(); }


void StateMachine::event_E10()
{ submitEvent(QByteArray::fromRawData("E10", 3)); }

void StateMachine::event_E100()
{ submitEvent(QByteArray::fromRawData("E100", 4)); }

void StateMachine::event_E13()
{ submitEvent(QByteArray::fromRawData("E13", 3)); }

void StateMachine::event_E16()
{ submitEvent(QByteArray::fromRawData("E16", 3)); }

void StateMachine::event_E17()
{ submitEvent(QByteArray::fromRawData("E17", 3)); }

void StateMachine::event_E19()
{ submitEvent(QByteArray::fromRawData("E19", 3)); }

void StateMachine::event_E21()
{ submitEvent(QByteArray::fromRawData("E21", 3)); }

void StateMachine::event_E22()
{ submitEvent(QByteArray::fromRawData("E22", 3)); }

void StateMachine::event_E25()
{ submitEvent(QByteArray::fromRawData("E25", 3)); }

void StateMachine::event_E28()
{ submitEvent(QByteArray::fromRawData("E28", 3)); }

void StateMachine::event_E29()
{ submitEvent(QByteArray::fromRawData("E29", 3)); }

void StateMachine::event_E33()
{ submitEvent(QByteArray::fromRawData("E33", 3)); }

void StateMachine::event_E37()
{ submitEvent(QByteArray::fromRawData("E37", 3)); }

void StateMachine::event_E38()
{ submitEvent(QByteArray::fromRawData("E38", 3)); }

void StateMachine::event_E47()
{ submitEvent(QByteArray::fromRawData("E47", 3)); }

void StateMachine::event_E49()
{ submitEvent(QByteArray::fromRawData("E49", 3)); }

void StateMachine::event_E52()
{ submitEvent(QByteArray::fromRawData("E52", 3)); }

void StateMachine::event_E54()
{ submitEvent(QByteArray::fromRawData("E54", 3)); }

void StateMachine::event_E55()
{ submitEvent(QByteArray::fromRawData("E55", 3)); }

void StateMachine::event_E56()
{ submitEvent(QByteArray::fromRawData("E56", 3)); }

void StateMachine::event_E58()
{ submitEvent(QByteArray::fromRawData("E58", 3)); }

void StateMachine::event_E65()
{ submitEvent(QByteArray::fromRawData("E65", 3)); }

void StateMachine::event_E68()
{ submitEvent(QByteArray::fromRawData("E68", 3)); }

void StateMachine::event_E70()
{ submitEvent(QByteArray::fromRawData("E70", 3)); }

void StateMachine::event_E75()
{ submitEvent(QByteArray::fromRawData("E75", 3)); }

void StateMachine::event_E77()
{ submitEvent(QByteArray::fromRawData("E77", 3)); }

void StateMachine::event_E8()
{ submitEvent(QByteArray::fromRawData("E8", 2)); }

void StateMachine::event_E80()
{ submitEvent(QByteArray::fromRawData("E80", 3)); }

void StateMachine::event_E82()
{ submitEvent(QByteArray::fromRawData("E82", 3)); }

void StateMachine::event_E83()
{ submitEvent(QByteArray::fromRawData("E83", 3)); }

void StateMachine::event_E85()
{ submitEvent(QByteArray::fromRawData("E85", 3)); }

void StateMachine::event_E86()
{ submitEvent(QByteArray::fromRawData("E86", 3)); }

void StateMachine::event_E88()
{ submitEvent(QByteArray::fromRawData("E88", 3)); }

void StateMachine::event_E89()
{ submitEvent(QByteArray::fromRawData("E89", 3)); }

void StateMachine::event_E93()
{ submitEvent(QByteArray::fromRawData("E93", 3)); }

void StateMachine::event_E97()
{ submitEvent(QByteArray::fromRawData("E97", 3)); }

void StateMachine::event_E99()
{ submitEvent(QByteArray::fromRawData("E99", 3)); }

qint32 StateMachine::Data::theInstructions[] = {
};

QVector<QString> StateMachine::Data::strings({
});

QVector<QByteArray> StateMachine::Data::byteArrays({
});

Scxml::ExecutableContent::StringIds StateMachine::Data::dataIds({
});

Scxml::EvaluatorInfos StateMachine::Data::evaluators({
});

Scxml::AssignmentInfos StateMachine::Data::assignments({
});

Scxml::ForeachInfos StateMachine::Data::foreaches({
});

