#include <qscxmllib/scxmlstatetable.h>

class StateMachine : public Scxml::StateTable {
    Q_OBJECT
public:
    StateMachine(QObject *parent = 0) : Scxml::StateTable(parent)
        , state_s_1(this)
        , state_s_2(this)
        , state_s_2_1(&state_s_2)
        , state_s_2_1_1(&state_s_2_1)
        , state_s_2_1_2(&state_s_2_1)
        , state_s_2_1_2_1(&state_s_2_1_2)
        , state_s_2_1_2_1_1(&state_s_2_1_2_1)
        , state_s_2_1_2_1_1_1(&state_s_2_1_2_1_1)
        , state_s_2_1_2_1_1_2(&state_s_2_1_2_1_1)
        , state_s_2_1_2_1_1_3(&state_s_2_1_2_1_1)
    {}

signals:
    void event_E10();
    void event_E100();
    void event_E13();
    void event_E16();
    void event_E17();
    void event_E19();
    void event_E21();
    void event_E22();
    void event_E25();
    void event_E28();
    void event_E29();
    void event_E33();
    void event_E37();
    void event_E38();
    void event_E47();
    void event_E49();
    void event_E52();
    void event_E54();
    void event_E55();
    void event_E56();
    void event_E58();
    void event_E65();
    void event_E68();
    void event_E70();
    void event_E75();
    void event_E77();
    void event_E8();
    void event_E80();
    void event_E82();
    void event_E83();
    void event_E85();
    void event_E86();
    void event_E88();
    void event_E89();
    void event_E93();
    void event_E97();
    void event_E99();
public:
public:
    bool init() Q_DECL_OVERRIDE {
        addId(QByteArray::fromRawData("s_1", 3), &state_s_1);
        addId(QByteArray::fromRawData("s_2", 3), &state_s_2);
        addId(QByteArray::fromRawData("s_2_1", 5), &state_s_2_1);
        addId(QByteArray::fromRawData("s_2_1_1", 7), &state_s_2_1_1);
        addId(QByteArray::fromRawData("s_2_1_2", 7), &state_s_2_1_2);
        addId(QByteArray::fromRawData("s_2_1_2_1", 9), &state_s_2_1_2_1);
        addId(QByteArray::fromRawData("s_2_1_2_1_1", 11), &state_s_2_1_2_1_1);
        addId(QByteArray::fromRawData("s_2_1_2_1_1_1", 13), &state_s_2_1_2_1_1_1);
        addId(QByteArray::fromRawData("s_2_1_2_1_1_2", 13), &state_s_2_1_2_1_1_2);
        addId(QByteArray::fromRawData("s_2_1_2_1_1_3", 13), &state_s_2_1_2_1_1_3);

        setInitialState(&state_s_1);
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E19", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E47", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E37", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E93", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E17", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }

        state_s_2.setInitialState(&state_s_2_1);

        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E85", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E49", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E28", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }

        state_s_2_1.setInitialState(&state_s_2_1_1);

        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E16", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E100", 4);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E70", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E52", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E33", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E89", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E22", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E56", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E68", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }

        state_s_2_1_2.setInitialState(&state_s_2_1_2_1);

        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E25", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E97", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E22", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E47", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E93", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E55", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }

        state_s_2_1_2_1.setInitialState(&state_s_2_1_2_1_1);

        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E80", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E19", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E22", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E65", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }

        state_s_2_1_2_1_1.setInitialState(&state_s_2_1_2_1_1_1);

        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E88", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E29", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E99", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E49", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E58", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E77", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E75", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E83", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E8", 2);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_1, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E37", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_1, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E28", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E8", 2);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E38", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E54", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_2, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E21", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E25", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2_1_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E10", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1_3);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E82", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2_1_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E99", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_1);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E86", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2);
            transition->init();
        }
        {
            QList<QByteArray> eventSelector;
            eventSelector << QByteArray::fromRawData("E13", 3);
            Scxml::ScxmlBaseTransition *transition = new Scxml::ScxmlBaseTransition(&state_s_2_1_2_1_1_3, eventSelector);
            transition->setTargetState(&state_s_2_1_2_1_1_2);
            transition->init();
        }
        return true;
    }
    Scxml::ScxmlState state_s_1;
    Scxml::ScxmlState state_s_2;
    Scxml::ScxmlState state_s_2_1;
    Scxml::ScxmlState state_s_2_1_1;
    Scxml::ScxmlState state_s_2_1_2;
    Scxml::ScxmlState state_s_2_1_2_1;
    Scxml::ScxmlState state_s_2_1_2_1_1;
    Scxml::ScxmlState state_s_2_1_2_1_1_1;
    Scxml::ScxmlState state_s_2_1_2_1_1_2;
    Scxml::ScxmlState state_s_2_1_2_1_1_3;
};
