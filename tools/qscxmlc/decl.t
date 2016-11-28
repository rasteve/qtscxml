class ${classname}: public QScxmlStateMachine
{
public:
    /* qmake ignore Q_OBJECT */
    Q_OBJECT
${properties}

public:
    Q_INVOKABLE ${classname}(QObject *parent = 0);
    ~${classname}();

private:
    struct Data;
    friend struct Data;
    struct Data *data;
};

