#ifndef OUT_H
#define OUT_H

#include <QScxmlLib/scxmlstatetable.h>

class StateMachine : public Scxml::StateTable
{
    Q_OBJECT

public:
    StateMachine(QObject *parent = 0);
    ~StateMachine();

    bool init() Q_DECL_OVERRIDE;

public slots:
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

private:
    struct Data;
    friend Data;
    struct Data *data;
};

#endif // OUT_H
