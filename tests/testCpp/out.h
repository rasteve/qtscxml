#include <qscxmllib/scxmlstatetable.h>

class StateMachine : public Scxml::StateTable {
    Q_OBJECT

public:
    StateMachine(QObject *parent = 0);
    ~StateMachine();

    bool init() Q_DECL_OVERRIDE;

public slots:
    void event_E10() { submitEvent(QByteArray::fromRawData("E10", 3)); }
    void event_E100() { submitEvent(QByteArray::fromRawData("E100", 4)); }
    void event_E13() { submitEvent(QByteArray::fromRawData("E13", 3)); }
    void event_E16() { submitEvent(QByteArray::fromRawData("E16", 3)); }
    void event_E17() { submitEvent(QByteArray::fromRawData("E17", 3)); }
    void event_E19() { submitEvent(QByteArray::fromRawData("E19", 3)); }
    void event_E21() { submitEvent(QByteArray::fromRawData("E21", 3)); }
    void event_E22() { submitEvent(QByteArray::fromRawData("E22", 3)); }
    void event_E25() { submitEvent(QByteArray::fromRawData("E25", 3)); }
    void event_E28() { submitEvent(QByteArray::fromRawData("E28", 3)); }
    void event_E29() { submitEvent(QByteArray::fromRawData("E29", 3)); }
    void event_E33() { submitEvent(QByteArray::fromRawData("E33", 3)); }
    void event_E37() { submitEvent(QByteArray::fromRawData("E37", 3)); }
    void event_E38() { submitEvent(QByteArray::fromRawData("E38", 3)); }
    void event_E47() { submitEvent(QByteArray::fromRawData("E47", 3)); }
    void event_E49() { submitEvent(QByteArray::fromRawData("E49", 3)); }
    void event_E52() { submitEvent(QByteArray::fromRawData("E52", 3)); }
    void event_E54() { submitEvent(QByteArray::fromRawData("E54", 3)); }
    void event_E55() { submitEvent(QByteArray::fromRawData("E55", 3)); }
    void event_E56() { submitEvent(QByteArray::fromRawData("E56", 3)); }
    void event_E58() { submitEvent(QByteArray::fromRawData("E58", 3)); }
    void event_E65() { submitEvent(QByteArray::fromRawData("E65", 3)); }
    void event_E68() { submitEvent(QByteArray::fromRawData("E68", 3)); }
    void event_E70() { submitEvent(QByteArray::fromRawData("E70", 3)); }
    void event_E75() { submitEvent(QByteArray::fromRawData("E75", 3)); }
    void event_E77() { submitEvent(QByteArray::fromRawData("E77", 3)); }
    void event_E8() { submitEvent(QByteArray::fromRawData("E8", 2)); }
    void event_E80() { submitEvent(QByteArray::fromRawData("E80", 3)); }
    void event_E82() { submitEvent(QByteArray::fromRawData("E82", 3)); }
    void event_E83() { submitEvent(QByteArray::fromRawData("E83", 3)); }
    void event_E85() { submitEvent(QByteArray::fromRawData("E85", 3)); }
    void event_E86() { submitEvent(QByteArray::fromRawData("E86", 3)); }
    void event_E88() { submitEvent(QByteArray::fromRawData("E88", 3)); }
    void event_E89() { submitEvent(QByteArray::fromRawData("E89", 3)); }
    void event_E93() { submitEvent(QByteArray::fromRawData("E93", 3)); }
    void event_E97() { submitEvent(QByteArray::fromRawData("E97", 3)); }
    void event_E99() { submitEvent(QByteArray::fromRawData("E99", 3)); }

private:
    struct Data;
    struct Data *data;
};
