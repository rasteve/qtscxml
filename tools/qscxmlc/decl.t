class ${classname}: public QScxmlStateMachine
{
public:
    /* qmake ignore Q_OBJECT */
    Q_OBJECT
${properties}

public:
    ${classname}(QObject *parent = 0);
    ~${classname}();

${getters}
signals:
${signals}
public slots:
${slots}
private:
    struct Data;
    friend struct Data;
    struct Data *data;
};

