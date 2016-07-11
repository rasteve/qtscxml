struct ${classname}::Data: private QScxmlTableData {
    Data(${classname} &stateMachine)
        : stateMachine(stateMachine)
    {}

    void init() {
        stateMachine.setTableData(this);
        ${dataModelInitialization}
    }

    QString name() const Q_DECL_OVERRIDE Q_DECL_FINAL
    { return ${name}; }

    QScxmlExecutableContent::ContainerId initialSetup() const Q_DECL_OVERRIDE Q_DECL_FINAL
    { return ${initialSetup}; }

    QScxmlExecutableContent::Instructions instructions() const Q_DECL_OVERRIDE Q_DECL_FINAL
    { return theInstructions; }

    QScxmlExecutableContent::StringId *dataNames(int *count) const Q_DECL_OVERRIDE Q_DECL_FINAL
    { *count = ${dataNameCount}; return dataIds; }

    QScxmlExecutableContent::EvaluatorInfo evaluatorInfo(QScxmlExecutableContent::EvaluatorId evaluatorId) const Q_DECL_OVERRIDE Q_DECL_FINAL
    { Q_ASSERT(evaluatorId >= 0); Q_ASSERT(evaluatorId < ${evaluatorCount}); return evaluators[evaluatorId]; }

    QScxmlExecutableContent::AssignmentInfo assignmentInfo(QScxmlExecutableContent::EvaluatorId assignmentId) const Q_DECL_OVERRIDE Q_DECL_FINAL
    { Q_ASSERT(assignmentId >= 0); Q_ASSERT(assignmentId < ${assignmentCount}); return assignments[assignmentId]; }

    QScxmlExecutableContent::ForeachInfo foreachInfo(QScxmlExecutableContent::EvaluatorId foreachId) const Q_DECL_OVERRIDE Q_DECL_FINAL
    { Q_ASSERT(foreachId >= 0); Q_ASSERT(foreachId < ${foreachCount}); return foreaches[foreachId]; }

    QString string(QScxmlExecutableContent::StringId id) const Q_DECL_OVERRIDE Q_DECL_FINAL
    {
        Q_ASSERT(id >= QScxmlExecutableContent::NoString); Q_ASSERT(id < ${stringCount});
        if (id == QScxmlExecutableContent::NoString) return QString();
        return QString({static_cast<QStringData*>(strings.data + id)});
    }

    const qint32 *stateMachineTable() const Q_DECL_OVERRIDE Q_DECL_FINAL
    { return theStateMachineTable; }

    QScxmlInvokableServiceFactory *serviceFactory(int id) const Q_DECL_OVERRIDE Q_DECL_FINAL;
    int signalIndexForEvent(const QString &event) const Q_DECL_OVERRIDE Q_DECL_FINAL;

    ${classname} &stateMachine;
    ${dataModelField}

    static qint32 theInstructions[];
    static QScxmlExecutableContent::StringId dataIds[];
    static QScxmlExecutableContent::EvaluatorInfo evaluators[];
    static QScxmlExecutableContent::AssignmentInfo assignments[];
    static QScxmlExecutableContent::ForeachInfo foreaches[];
    static const qint32 theStateMachineTable[];
    static struct Strings {
        QArrayData data[${stringCount}];
        qunicodechar stringdata[${stringdataSize}];
    } strings;

    static std::vector<QString> outgoingEvents;
};

${classname}::${classname}(QObject *parent)
    : QScxmlStateMachine(&staticMetaObject, parent)
    , data(new Data(*this))
{ qRegisterMetaType<${classname} *>(); data->init(); }

${classname}::~${classname}()
{ delete data; }

QScxmlInvokableServiceFactory *${classname}::Data::serviceFactory(int id) const
{
${serviceFactories}
}

std::vector<QString> ${classname}::Data::outgoingEvents = {
${outgoingEvents}
};

int ${classname}::Data::signalIndexForEvent(const QString &event) const
{
    auto it = std::lower_bound(outgoingEvents.begin(), outgoingEvents.end(), event);
    if (it != outgoingEvents.end() && *it == event) {
        return int(std::distance(outgoingEvents.begin(), it));
    } else {
        return -1;
    }
}

qint32 ${classname}::Data::theInstructions[] = {
${theInstructions}
};

QScxmlExecutableContent::StringId ${classname}::Data::dataIds[] = {
${dataIds}
};

QScxmlExecutableContent::EvaluatorInfo ${classname}::Data::evaluators[] = {
${evaluators}
};

QScxmlExecutableContent::AssignmentInfo ${classname}::Data::assignments[] = {
${assignments}
};

QScxmlExecutableContent::ForeachInfo ${classname}::Data::foreaches[] = {
${foreaches}
};

#define STR_LIT(idx, ofs, len) \
    Q_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(Strings, stringdata) + ofs * sizeof(qunicodechar) - idx * sizeof(QArrayData)) \
    )
${classname}::Data::Strings ${classname}::Data::strings = {{
${strLits}
},{
${uniLits}
}};

const qint32 ${classname}::Data::theStateMachineTable[] = ${theStateMachineTable};

${metaObject}
${getters}
${slots}
