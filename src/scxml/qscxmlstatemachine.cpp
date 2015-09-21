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

#include "qscxmlstatemachine_p.h"
#include "qscxmlexecutablecontent_p.h"
#include "qscxmlevent_p.h"
#include "qscxmlqstates_p.h"

#include <QAbstractState>
#include <QAbstractTransition>
#include <QFile>
#include <QHash>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QState>
#include <QString>
#include <QTimer>

#include <QtCore/private/qstatemachine_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(scxmlLog, "scxml.table")

namespace {
class InvokableScxml: public QScxmlInvokableService
{
public:
    InvokableScxml(QScxmlStateMachine *stateMachine, const QString &id, const QVariantMap &data,
                   bool autoforward, QScxmlExecutableContent::ContainerId finalize, QScxmlStateMachine *parent)
        : QScxmlInvokableService(id, data, autoforward, finalize, parent)
        , m_stateMachine(stateMachine)
    {
        stateMachine->setSessionId(id);
        stateMachine->setParentStateMachine(parent);
        stateMachine->init(data);
        stateMachine->start();
    }

    ~InvokableScxml()
    { delete m_stateMachine; }

    void submitEvent(QScxmlEvent *event) Q_DECL_OVERRIDE
    {
        m_stateMachine->submitEvent(event);
    }

private:
    QScxmlStateMachine *m_stateMachine;
};
} // anonymous namespace

namespace QScxmlInternal {
class WrappedQStateMachinePrivate: public QStateMachinePrivate
{
    Q_DECLARE_PUBLIC(WrappedQStateMachine)

public:
    WrappedQStateMachinePrivate(QScxmlStateMachine *table)
        : m_stateMachine(table)
        , m_queuedEvents(Q_NULLPTR)
    {}
    ~WrappedQStateMachinePrivate()
    { delete m_queuedEvents; }

    int eventIdForDelayedEvent(const QByteArray &scxmlEventId);

    QScxmlStateMachine *stateMachine() const
    { return m_stateMachine; }

    QScxmlStateMachinePrivate *stateMachinePrivate() const
    { return QScxmlStateMachinePrivate::get(stateMachine()); }

protected: // overrides for QStateMachinePrivate:
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    void noMicrostep() Q_DECL_OVERRIDE;
    void processedPendingEvents(bool didChange) Q_DECL_OVERRIDE;
    void beginMacrostep() Q_DECL_OVERRIDE;
    void endMacrostep(bool didChange) Q_DECL_OVERRIDE;

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void enterStates(QEvent *event, const QList<QAbstractState*> &exitedStates_sorted,
                     const QList<QAbstractState*> &statesToEnter_sorted,
                     const QSet<QAbstractState*> &statesForDefaultEntry,
                     QHash<QAbstractState *, QVector<QPropertyAssignment> > &propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
                     , const QList<QAbstractAnimation*> &selectedAnimations
#  endif
                     ) Q_DECL_OVERRIDE;
    void exitStates(QEvent *event, const QList<QAbstractState *> &statesToExit_sorted,
                    const QHash<QAbstractState*, QVector<QPropertyAssignment> > &assignmentsForEnteredStates) Q_DECL_OVERRIDE;

    void exitInterpreter() Q_DECL_OVERRIDE;
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)

    void emitStateFinished(QState *forState, QFinalState *guiltyState) Q_DECL_OVERRIDE;
    void startupHook() Q_DECL_OVERRIDE;
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

public: // fields
    QScxmlStateMachine *m_stateMachine;

    struct QueuedEvent
    {
        QueuedEvent(QEvent *event = Q_NULLPTR, QStateMachine::EventPriority priority = QStateMachine::NormalPriority)
            : event(event)
            , priority(priority)
        {}

        QEvent *event;
        QStateMachine::EventPriority priority;
    };
    QVector<QueuedEvent> *m_queuedEvents;
};

WrappedQStateMachine::WrappedQStateMachine(QScxmlStateMachine *parent)
    : QStateMachine(*new WrappedQStateMachinePrivate(parent), parent)
{}

WrappedQStateMachine::WrappedQStateMachine(WrappedQStateMachinePrivate &dd, QScxmlStateMachine *parent)
    : QStateMachine(dd, parent)
{}

QScxmlStateMachine *WrappedQStateMachine::stateMachine() const
{
    Q_D(const WrappedQStateMachine);

    return d->stateMachine();
}

QScxmlStateMachinePrivate *WrappedQStateMachine::stateMachinePrivate()
{
    Q_D(const WrappedQStateMachine);

    return d->stateMachinePrivate();
}
} // Internal namespace

class QScxmlError::ScxmlErrorPrivate
{
public:
    ScxmlErrorPrivate()
        : line(-1)
        , column(-1)
    {}

    QString fileName;
    int line;
    int column;
    QString description;
};

QScxmlError::QScxmlError()
    : d(Q_NULLPTR)
{}

QScxmlError::QScxmlError(const QString &fileName, int line, int column, const QString &description)
    : d(new ScxmlErrorPrivate)
{
    d->fileName = fileName;
    d->line = line;
    d->column = column;
    d->description = description;
}

QScxmlError::QScxmlError(const QScxmlError &other)
    : d(Q_NULLPTR)
{
    *this = other;
}

QScxmlError &QScxmlError::operator=(const QScxmlError &other)
{
    if (other.d) {
        if (!d)
            d = new ScxmlErrorPrivate;
        d->fileName = other.d->fileName;
        d->line = other.d->line;
        d->column = other.d->column;
        d->description = other.d->description;
    } else {
        delete d;
        d = Q_NULLPTR;
    }
    return *this;
}

QScxmlError::~QScxmlError()
{
    delete d;
    d = Q_NULLPTR;
}

bool QScxmlError::isValid() const
{
    return d != Q_NULLPTR;
}

QString QScxmlError::fileName() const
{
    return isValid() ? d->fileName : QString();
}

int QScxmlError::line() const
{
    return isValid() ? d->line : -1;
}

int QScxmlError::column() const
{
    return isValid() ? d->column : -1;
}

QString QScxmlError::description() const
{
    return isValid() ? d->description : QString();
}

QString QScxmlError::toString() const
{
    QString str;
    if (!isValid())
        return str;

    if (d->fileName.isEmpty())
        str = QStringLiteral("<Unknown File>");
    else
        str = d->fileName;
    if (d->line != -1) {
        str += QStringLiteral(":%1").arg(d->line);
        if (d->column != -1)
            str += QStringLiteral(":%1").arg(d->column);
    }
    str += QStringLiteral(": error: ") + d->description;

    return str;
}

QDebug operator<<(QDebug debug, const QScxmlError &error)
{
    debug << error.toString();
    return debug;
}

QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<QScxmlError> &errors)
{
    foreach (const QScxmlError &error, errors) {
        debug << error << endl;
    }
    return debug;
}

QScxmlTableData::~QScxmlTableData()
{}

class QScxmlInvokableService::Data
{
public:
    Data(const QString &id, const QVariantMap &data, bool autoforward,
         QScxmlExecutableContent::ContainerId finalize, QScxmlStateMachine *parent)
        : id(id)
        , data(data)
        , autoforward(autoforward)
        , parent(parent)
        , finalize(finalize)
    {}

    QString id;
    QVariantMap data;
    bool autoforward;
    QScxmlStateMachine *parent;
    QScxmlExecutableContent::ContainerId finalize;
};

QScxmlInvokableService::QScxmlInvokableService(const QString &id,
                                             const QVariantMap &data,
                                             bool autoforward,
                                             QScxmlExecutableContent::ContainerId finalize,
                                             QScxmlStateMachine *parent)
    : d(new Data(id, data, autoforward, finalize, parent))
{}

QScxmlInvokableService::~QScxmlInvokableService()
{
    delete d;
}

QString QScxmlInvokableService::id() const
{
    return d->id;
}

bool QScxmlInvokableService::autoforward() const
{
    return d->autoforward;
}

QVariantMap QScxmlInvokableService::data() const
{
    return d->data;
}

QScxmlStateMachine *QScxmlInvokableService::parent() const
{
    return d->parent;
}

void QScxmlInvokableService::finalize()
{
    auto smp = QScxmlStateMachinePrivate::get(parent());
    smp->m_executionEngine->execute(d->finalize);
}

class QScxmlInvokableServiceFactory::Data
{
public:
    Data(QScxmlExecutableContent::StringId invokeLocation, QScxmlExecutableContent::StringId id,
         QScxmlExecutableContent::StringId idPrefix, QScxmlExecutableContent::StringId idlocation,
         const QVector<QScxmlExecutableContent::StringId> &namelist, bool autoforward,
         const QVector<QScxmlInvokableServiceFactory::Param> &params,
         QScxmlExecutableContent::ContainerId finalize)
        : invokeLocation(invokeLocation)
        , id(id)
        , idPrefix(idPrefix)
        , idlocation(idlocation)
        , namelist(namelist)
        , autoforward(autoforward)
        , params(params)
        , finalize(finalize)
    {}

    QScxmlExecutableContent::StringId invokeLocation;
    QScxmlExecutableContent::StringId id;
    QScxmlExecutableContent::StringId idPrefix;
    QScxmlExecutableContent::StringId idlocation;
    QVector<QScxmlExecutableContent::StringId> namelist;
    bool autoforward;
    QVector<QScxmlInvokableServiceFactory::Param> params;
    QScxmlExecutableContent::ContainerId finalize;

};

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,  QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix, QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist, bool autoforward,
        const QVector<Param> &params, QScxmlExecutableContent::ContainerId finalize)
    : d(new Data(invokeLocation, id, idPrefix, idlocation, namelist, autoforward, params, finalize))
{}

QScxmlInvokableServiceFactory::~QScxmlInvokableServiceFactory()
{
    delete d;
}

QString QScxmlInvokableServiceFactory::calculateId(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto table = parent->tableData();

    if (d->id != QScxmlExecutableContent::NoString) {
        return table->string(d->id);
    }

    QString id = QScxmlStateMachine::generateSessionId(table->string(d->idPrefix));

    if (d->idlocation != QScxmlExecutableContent::NoString) {
        auto idloc = table->string(d->idlocation);
        auto ctxt = table->string(d->invokeLocation);
        parent->dataModel()->setProperty(idloc, id, ctxt, ok);
        if (!*ok)
            return QString();
    }

    return id;
}

QVariantMap QScxmlInvokableServiceFactory::calculateData(QScxmlStateMachine *parent, bool *ok) const
{
    Q_ASSERT(ok);

    QVariantMap result;
    auto dataModel = parent->dataModel();
    auto tableData = parent->tableData();

    foreach (const Param &param, d->params) {
        auto name = tableData->string(param.name);

        if (param.expr != QScxmlExecutableContent::NoEvaluator) {
            *ok = false;
            auto v = dataModel->evaluateToVariant(param.expr, ok);
            if (!*ok)
                return QVariantMap();
            result.insert(name, v);
        } else {
            QString loc;
            if (param.location != QScxmlExecutableContent::NoString) {
                loc = tableData->string(param.location);
            }

            if (loc.isEmpty()) {
                // TODO: error message?
                *ok = false;
                return QVariantMap();
            }

            auto v = dataModel->property(loc);
            result.insert(name, v);
        }
    }

    foreach (QScxmlExecutableContent::StringId locid, d->namelist) {
        QString loc;
        if (locid != QScxmlExecutableContent::NoString) {
            loc = tableData->string(locid);
        }
        if (loc.isEmpty()) {
            // TODO: error message?
            *ok = false;
            return QVariantMap();
        }
        if (dataModel->hasProperty(loc)) {
            auto v = dataModel->property(loc);
            result.insert(loc, v);
        } else {
            *ok = false;
            return QVariantMap();
        }
    }

    return result;
}

bool QScxmlInvokableServiceFactory::autoforward() const
{
    return d->autoforward;
}

QScxmlExecutableContent::ContainerId QScxmlInvokableServiceFactory::finalizeContent() const
{
    return d->finalize;
}

QScxmlEventFilter::~QScxmlEventFilter()
{}

QScxmlInvokableScxmlServiceFactory::QScxmlInvokableScxmlServiceFactory(
        QScxmlExecutableContent::StringId invokeLocation,
        QScxmlExecutableContent::StringId id,
        QScxmlExecutableContent::StringId idPrefix,
        QScxmlExecutableContent::StringId idlocation,
        const QVector<QScxmlExecutableContent::StringId> &namelist,
        bool doAutoforward,
        const QVector<QScxmlInvokableServiceFactory::Param> &params,
        QScxmlExecutableContent::ContainerId finalize)
    : QScxmlInvokableServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist,
                                   doAutoforward, params, finalize)
{}

QScxmlInvokableService *QScxmlInvokableScxmlServiceFactory::finishInvoke(QScxmlStateMachine *child, QScxmlStateMachine *parent)
{
    bool ok = false;
    auto id = calculateId(parent, &ok);
    if (!ok)
        return Q_NULLPTR;
    auto data = calculateData(parent, &ok);
    if (!ok)
        return Q_NULLPTR;
    child->setIsInvoked(true);
    return new InvokableScxml(child, id, data, autoforward(), finalizeContent(), parent);
}

QAtomicInt QScxmlStateMachinePrivate::m_sessionIdCounter = QAtomicInt(0);

QScxmlStateMachinePrivate::QScxmlStateMachinePrivate()
    : QObjectPrivate()
    , m_sessionId(QScxmlStateMachine::generateSessionId(QStringLiteral("session-")))
    , m_isInvoked(false)
    , m_dataModel(Q_NULLPTR)
    , m_dataBinding(QScxmlStateMachine::EarlyBinding)
    , m_executionEngine(Q_NULLPTR)
    , m_tableData(Q_NULLPTR)
    , m_qStateMachine(Q_NULLPTR)
    , m_eventFilter(Q_NULLPTR)
    , m_parentStateMachine(Q_NULLPTR)
{}

QScxmlStateMachinePrivate::~QScxmlStateMachinePrivate()
{
    delete m_executionEngine;
}

void QScxmlStateMachinePrivate::setQStateMachine(QScxmlInternal::WrappedQStateMachine *stateMachine)
{
    m_qStateMachine = stateMachine;
}

static QScxmlState *findState(const QString &scxmlName, QStateMachine *parent)
{
    QList<QObject *> worklist;
    worklist.reserve(parent->children().size() + parent->configuration().size());
    worklist.append(parent);

    while (!worklist.isEmpty()) {
        QObject *obj = worklist.takeLast();
        if (QScxmlState *state = qobject_cast<QScxmlState *>(obj)) {
            if (state->objectName() == scxmlName)
                return state;
        }
        worklist.append(obj->children());
    }

    return Q_NULLPTR;
}

QAbstractState *QScxmlStateMachinePrivate::stateByScxmlName(const QString &scxmlName)
{
    return findState(scxmlName, m_qStateMachine);
}

QScxmlStateMachinePrivate::ParserData *QScxmlStateMachinePrivate::parserData()
{
    if (m_parserData.isNull())
        m_parserData.reset(new ParserData);
    return m_parserData.data();
}

QScxmlStateMachine *QScxmlStateMachine::fromFile(const QString &fileName, QScxmlDataModel *dataModel)
{
    QFile scxmlFile(fileName);
    if (!scxmlFile.open(QIODevice::ReadOnly)) {
        QVector<QScxmlError> errors({
                                QScxmlError(scxmlFile.fileName(), 0, 0, QStringLiteral("cannot open for reading"))
                            });
        auto table = new QScxmlStateMachine;
        QScxmlStateMachinePrivate::get(table)->parserData()->m_errors = errors;
        return table;
    }

    QScxmlStateMachine *table = fromData(&scxmlFile, fileName, dataModel);
    scxmlFile.close();
    return table;
}

QScxmlStateMachine *QScxmlStateMachine::fromData(QIODevice *data, const QString &fileName, QScxmlDataModel *dataModel)
{
    QXmlStreamReader xmlReader(data);
    QScxmlParser parser(&xmlReader);
    parser.setFileName(fileName);
    parser.parse();
    auto table = parser.instantiateStateMachine();
    if (dataModel) {
        table->setDataModel(dataModel);
    } else {
        parser.instantiateDataModel(table);
    }
    return table;
}

QVector<QScxmlError> QScxmlStateMachine::errors() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_parserData ? d->m_parserData->m_errors : QVector<QScxmlError>();
}

QScxmlStateMachine::QScxmlStateMachine(QObject *parent)
    : QObject(*new QScxmlStateMachinePrivate, parent)
{
    Q_D(QScxmlStateMachine);
    d->m_executionEngine = new QScxmlExecutableContent::ExecutionEngine(this);
    d->setQStateMachine(new QScxmlInternal::WrappedQStateMachine(this));
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::onFinished);
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::finished);
}

QScxmlStateMachine::QScxmlStateMachine(QScxmlStateMachinePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QScxmlStateMachine);
    d->m_executionEngine = new QScxmlExecutableContent::ExecutionEngine(this);
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::onFinished);
    connect(d->m_qStateMachine, &QStateMachine::finished, this, &QScxmlStateMachine::finished);
}

QString QScxmlStateMachine::sessionId() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_sessionId;
}

void QScxmlStateMachine::setSessionId(const QString &id)
{
    Q_D(QScxmlStateMachine);
    d->m_sessionId = id;
}

QString QScxmlStateMachine::generateSessionId(const QString &prefix)
{
    int id = ++QScxmlStateMachinePrivate::m_sessionIdCounter;
    return prefix + QString::number(id);
}

bool QScxmlStateMachine::isInvoked() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_isInvoked;
}

void QScxmlStateMachine::setIsInvoked(bool invoked)
{
    Q_D(QScxmlStateMachine);
    d->m_isInvoked = invoked;
}

QScxmlDataModel *QScxmlStateMachine::dataModel() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_dataModel;
}

void QScxmlStateMachine::setDataModel(QScxmlDataModel *dataModel)
{
    Q_D(QScxmlStateMachine);

    d->m_dataModel = dataModel;
    dataModel->setTable(this);
}

void QScxmlStateMachine::setDataBinding(QScxmlStateMachine::BindingMethod b)
{
    Q_D(QScxmlStateMachine);

    d->m_dataBinding = b;
}

QScxmlStateMachine::BindingMethod QScxmlStateMachine::dataBinding() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_dataBinding;
}

QScxmlTableData *QScxmlStateMachine::tableData() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_tableData;
}

void QScxmlStateMachine::setTableData(QScxmlTableData *tableData)
{
    Q_D(QScxmlStateMachine);

    d->m_tableData = tableData;
}

void QScxmlStateMachine::doLog(const QString &label, const QString &msg)
{
    qCDebug(scxmlLog) << label << ":" << msg;
    emit log(label, msg);
}

QScxmlStateMachine *QScxmlStateMachine::parentStateMachine() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_parentStateMachine;
}

void QScxmlStateMachine::setParentStateMachine(QScxmlStateMachine *parent)
{
    Q_D(QScxmlStateMachine);
    d->m_parentStateMachine = parent;
}

void QScxmlInternal::WrappedQStateMachine::beginSelectTransitions(QEvent *event)
{
    Q_D(WrappedQStateMachine);

    { // handle <invoke>s
        QVector<QScxmlState*> &sti = d->stateMachinePrivate()->m_statesToInvoke;
        std::sort(sti.begin(), sti.end(), WrappedQStateMachinePrivate::stateEntryLessThan);
        foreach (QScxmlState *s, sti) {
            auto sp = QScxmlStatePrivate::get(s);
            foreach (QScxmlInvokableServiceFactory *f, sp->invokableServiceFactories) {
                if (auto service = f->invoke(stateMachine())) {
                    sp->invokedServices.append(service);
                    stateMachinePrivate()->m_invokedServices.append(service);
                }
            }
        }
        sti.clear();
    }

    if (event && event->type() == QScxmlEvent::scxmlEventType) {
        stateMachinePrivate()->m_event = *static_cast<QScxmlEvent *>(event);
        d->stateMachine()->dataModel()->setEvent(stateMachinePrivate()->m_event);

        auto scxmlEvent = static_cast<QScxmlEvent *>(event);
        auto smp = stateMachinePrivate();

        foreach (QScxmlInvokableService *service, smp->m_invokedServices) {
            if (scxmlEvent->invokeId() == service->id()) {
                service->finalize();
            }
            if (service->autoforward()) {
                qCDebug(scxmlLog) << "auto-forwarding event" << scxmlEvent->name()
                                  << "from" << stateMachine()->name() << "to service" << service->id();
                service->submitEvent(new QScxmlEvent(*scxmlEvent));
            }
        }

        auto e = static_cast<QScxmlEvent *>(event);
        if (smp->m_eventFilter && !smp->m_eventFilter->handle(e, d->stateMachine())) {
            e->makeIgnorable();
            e->clear();
            smp->m_event.clear();
            return;
        }
    } else {
        stateMachinePrivate()->m_event.clear();
        d->stateMachine()->dataModel()->setEvent(stateMachinePrivate()->m_event);
    }
}

void QScxmlInternal::WrappedQStateMachine::beginMicrostep(QEvent *event)
{
    Q_D(WrappedQStateMachine);

    qCDebug(scxmlLog) << d->m_stateMachine->tableData()->name() << " started microstep from state" << d->m_stateMachine->activeStates()
                      << "with event" << stateMachinePrivate()->m_event.name() << "from event type" << event->type();
}

void QScxmlInternal::WrappedQStateMachine::endMicrostep(QEvent *event)
{
    Q_D(WrappedQStateMachine);
    Q_UNUSED(event);

    qCDebug(scxmlLog) << d->m_stateMachine->tableData()->name() << " finished microstep in state (" << d->m_stateMachine->activeStates() << ")";
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

void QScxmlInternal::WrappedQStateMachinePrivate::noMicrostep()
{
    qCDebug(scxmlLog) << m_stateMachine->tableData()->name() << " had no transition, stays in state (" << m_stateMachine->activeStates() << ")";
}

void QScxmlInternal::WrappedQStateMachinePrivate::processedPendingEvents(bool didChange)
{
    qCDebug(scxmlLog) << m_stateMachine->tableData()->name() << " finishedPendingEvents " << didChange << " in state ("
                      << m_stateMachine->activeStates() << ")";
    emit m_stateMachine->reachedStableState(didChange);
}

void QScxmlInternal::WrappedQStateMachinePrivate::beginMacrostep()
{
}

void QScxmlInternal::WrappedQStateMachinePrivate::endMacrostep(bool didChange)
{
    qCDebug(scxmlLog) << m_stateMachine->tableData()->name() << " endMacrostep " << didChange << " in state ("
                      << m_stateMachine->activeStates() << ")";
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void QScxmlInternal::WrappedQStateMachinePrivate::enterStates(
        QEvent *event,
        const QList<QAbstractState*> &exitedStates_sorted,
        const QList<QAbstractState*> &statesToEnter_sorted,
        const QSet<QAbstractState*> &statesForDefaultEntry,
        QHash<QAbstractState *, QVector<QPropertyAssignment> > &propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
        , const QList<QAbstractAnimation*> &selectedAnimations
#  endif
        )
{
    QStateMachinePrivate::enterStates(event, exitedStates_sorted, statesToEnter_sorted,
                                      statesForDefaultEntry, propertyAssignmentsForState
#  ifndef QT_NO_ANIMATION
                                      , selectedAnimations
#  endif
                                      );
    foreach (QAbstractState *s, statesToEnter_sorted) {
        if (QScxmlState *qss = qobject_cast<QScxmlState *>(s)) {
            if (!QScxmlStatePrivate::get(qss)->invokableServiceFactories.isEmpty()) {
                if (!stateMachinePrivate()->m_statesToInvoke.contains(qss)) {
                    stateMachinePrivate()->m_statesToInvoke.append(qss);
                }
            }
        }
    }
}

void QScxmlInternal::WrappedQStateMachinePrivate::exitStates(
        QEvent *event,
        const QList<QAbstractState *> &statesToExit_sorted,
        const QHash<QAbstractState*, QVector<QPropertyAssignment> > &assignmentsForEnteredStates)
{
    QStateMachinePrivate::exitStates(event, statesToExit_sorted, assignmentsForEnteredStates);

    auto smp = stateMachinePrivate();
    for (int i = 0; i < smp->m_statesToInvoke.size(); ) {
        if (statesToExit_sorted.contains(smp->m_statesToInvoke.at(i))) {
            smp->m_statesToInvoke.removeAt(i);
        } else {
            ++i;
        }
    }

    foreach (QAbstractState *s, statesToExit_sorted) {
        if (QScxmlState *qss = qobject_cast<QScxmlState *>(s)) {
            QVector<QScxmlInvokableService *> &services = QScxmlStatePrivate::get(qss)->invokedServices;
            foreach (QScxmlInvokableService *service, services) {
                qCDebug(scxmlLog) << stateMachine()->name() << "canceling service" << service->id();
                stateMachinePrivate()->m_invokedServices.removeOne(service);
                delete service;
            }
            services.clear();
        }
    }
}

void QScxmlInternal::WrappedQStateMachinePrivate::exitInterpreter()
{
    Q_Q(WrappedQStateMachine);

    foreach (QAbstractState *s, configuration) {
        QScxmlExecutableContent::ContainerId onExitInstructions = QScxmlExecutableContent::NoInstruction;
        if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(s)) {
            stateMachinePrivate()->m_executionEngine->execute(finalState->doneData(), QVariant());
            onExitInstructions = QScxmlFinalState::Data::get(finalState)->onExitInstructions;
        } else if (QScxmlState *state = qobject_cast<QScxmlState *>(s)) {
            onExitInstructions = QScxmlStatePrivate::get(state)->onExitInstructions;
        }

        if (onExitInstructions != QScxmlExecutableContent::NoInstruction) {
            stateMachinePrivate()->m_executionEngine->execute(onExitInstructions);
        }

        if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(s)) {
            if (finalState->parent() == q) {
                auto psm = m_stateMachine->parentStateMachine();
                if (psm) {
                    auto done = new QScxmlEvent;
                    done->setName(QByteArray("done.invoke.") + m_stateMachine->sessionId().toUtf8());
                    done->setInvokeId(m_stateMachine->sessionId());
                    qCDebug(scxmlLog) << "submitting event" << done->name() << "to" << psm->name();
                    psm->submitEvent(done);
                }
            }
        }
    }
}
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)

void QScxmlInternal::WrappedQStateMachinePrivate::emitStateFinished(QState *forState, QFinalState *guiltyState)
{
    Q_Q(WrappedQStateMachine);

    if (QScxmlFinalState *finalState = qobject_cast<QScxmlFinalState *>(guiltyState)) {
        if (!q->isRunning())
            return;
        stateMachinePrivate()->m_executionEngine->execute(finalState->doneData(), forState->objectName());
    }

    QStateMachinePrivate::emitStateFinished(forState, guiltyState);
}

void QScxmlInternal::WrappedQStateMachinePrivate::startupHook()
{
    Q_Q(WrappedQStateMachine);

    q->submitQueuedEvents();
}

#endif // QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

int QScxmlInternal::WrappedQStateMachinePrivate::eventIdForDelayedEvent(const QByteArray &scxmlEventId)
{
    QMutexLocker locker(&delayedEventsMutex);

    QHash<int, DelayedEvent>::const_iterator it;
    for (it = delayedEvents.constBegin(); it != delayedEvents.constEnd(); ++it) {
        if (QScxmlEvent *e = dynamic_cast<QScxmlEvent *>(it->event)) {
            if (e->sendId() == scxmlEventId) {
                return it.key();
            }
        }
    }

    return -1;
}

QStringList QScxmlStateMachine::activeStates(bool compress)
{
    Q_D(QScxmlStateMachine);

    QSet<QAbstractState *> config = QStateMachinePrivate::get(d->m_qStateMachine)->configuration;
    if (compress)
        foreach (const QAbstractState *s, config)
            config.remove(s->parentState());
    QStringList res;
    foreach (const QAbstractState *s, config) {
        QString id = s->objectName();
        if (!id.isEmpty()) {
            res.append(id);
        }
    }
    std::sort(res.begin(), res.end());
    return res;
}

bool QScxmlStateMachine::isActive(const QString &scxmlStateName) const
{
    Q_D(const QScxmlStateMachine);
    QSet<QAbstractState *> config = QStateMachinePrivate::get(d->m_qStateMachine)->configuration;
    foreach (QAbstractState *s, config) {
        if (s->objectName() == scxmlStateName) {
            return true;
        }
    }
    return false;
}

bool QScxmlStateMachine::hasState(const QString &scxmlStateName) const
{
    Q_D(const QScxmlStateMachine);
    return findState(scxmlStateName, d->m_qStateMachine) != Q_NULLPTR;
}

QMetaObject::Connection QScxmlStateMachine::connect(const QString &scxmlStateName, const char *signal,
                                            const QObject *receiver, const char *method,
                                            Qt::ConnectionType type)
{
    Q_D(QScxmlStateMachine);
    QScxmlState *state = findState(scxmlStateName, d->m_qStateMachine);
    return QObject::connect(state, signal, receiver, method, type);
}

QScxmlEventFilter *QScxmlStateMachine::scxmlEventFilter() const
{
    Q_D(const QScxmlStateMachine);
    return d->m_eventFilter;
}

void QScxmlStateMachine::setScxmlEventFilter(QScxmlEventFilter *newFilter)
{
    Q_D(QScxmlStateMachine);
    d->m_eventFilter = newFilter;
}

void QScxmlStateMachine::executeInitialSetup()
{
    Q_D(const QScxmlStateMachine);
    d->m_executionEngine->execute(tableData()->initialSetup());
}

static bool loopOnSubStates(QState *startState,
                            std::function<bool(QState *)> enteringState = Q_NULLPTR,
                            std::function<void(QState *)> exitingState = Q_NULLPTR,
                            std::function<void(QAbstractState *)> inAbstractState = Q_NULLPTR)
{
    QList<int> pos;
    QState *parentAtt = startState;
    QObjectList childs = startState->children();
    pos << 0;
    while (!pos.isEmpty()) {
        bool goingDeeper = false;
        for (int i = pos.last(); i < childs.size() ; ++i) {
            if (QAbstractState *as = qobject_cast<QAbstractState *>(childs.at(i))) {
                if (QState *s = qobject_cast<QState *>(as)) {
                    if (enteringState && !enteringState(s))
                        continue;
                    pos.last() = i + 1;
                    parentAtt = s;
                    childs = s->children();
                    pos << 0;
                    goingDeeper = !childs.isEmpty();
                    break;
                } else if (inAbstractState) {
                    inAbstractState(as);
                }
            }
        }
        if (!goingDeeper) {
            do {
                pos.removeLast();
                if (pos.isEmpty())
                    break;
                if (exitingState)
                    exitingState(parentAtt);
                parentAtt = parentAtt->parentState();
                childs = parentAtt->children();
            } while (!pos.isEmpty() && pos.last() >= childs.size());
        }
    }
    return true;
}

bool QScxmlStateMachine::init(const QVariantMap &initialDataValues)
{
    Q_D(QScxmlStateMachine);

    dataModel()->setup(initialDataValues);
    executeInitialSetup();

    bool res = true;
    loopOnSubStates(d->m_qStateMachine, std::function<bool(QState *)>(), [&res](QState *state) {
        if (QScxmlState *s = qobject_cast<QScxmlState *>(state))
            if (!s->init())
                res = false;
        if (QScxmlFinalState *s = qobject_cast<QScxmlFinalState *>(state))
            if (!s->init())
                res = false;
        foreach (QAbstractTransition *t, state->transitions()) {
            if (QScxmlTransition *scTransition = qobject_cast<QScxmlTransition *>(t))
                if (!scTransition->init())
                    res = false;
        }
    });
    foreach (QAbstractTransition *t, d->m_qStateMachine->transitions()) {
        if (QScxmlTransition *scTransition = qobject_cast<QScxmlTransition *>(t))
            if (!scTransition->init())
                res = false;
    }
    return res;
}

bool QScxmlStateMachine::isRunning() const
{
    Q_D(const QScxmlStateMachine);

    return d->m_qStateMachine->isRunning();
}

QString QScxmlStateMachine::name() const
{
    return tableData()->name();
}

void QScxmlStateMachine::submitError(const QByteArray &type, const QString &msg, const QByteArray &sendid)
{
    qCDebug(scxmlLog) << "machine" << name() << "had error" << type << ":" << msg;
    submitEvent(EventBuilder::errorEvent(this, type, sendid));
}

void QScxmlStateMachine::routeEvent(QScxmlEvent *e)
{
    Q_D(QScxmlStateMachine);

    if (!e)
        return;

    QString origin = e->origin();
    if (origin == QStringLiteral("#_parent")) {
        if (auto psm = parentStateMachine()) {
            qCDebug(scxmlLog) << "routing event" << e->name() << "from" << name() << "to parent" << psm->name();
            psm->submitEvent(e);
        } else {
            qCDebug(scxmlLog) << "machine" << name() << "is not invoked, so it cannot route a message to #_parent";
            delete e;
        }
    } else if (origin.startsWith(QStringLiteral("#_")) && origin != QStringLiteral("#_internal")) {
        // route to children
        auto originId = origin.midRef(2);
        foreach (QScxmlInvokableService *service, d->m_invokedServices) {
            if (service->id() == originId) {
                qCDebug(scxmlLog) << "routing event" << e->name() << "from" << name() << "to parent" << service->id();
                service->submitEvent(new QScxmlEvent(*e));
            }
        }
        delete e;
    } else {
        submitEvent(e);
    }
}

void QScxmlStateMachine::submitEvent(QScxmlEvent *e)
{
    Q_D(QScxmlStateMachine);

    if (!e)
        return;

    if (e->delay() > 0) {
        qCDebug(scxmlLog) << name() << ": submitting event" << e->name() << "with delay" << e->delay() << "ms" << "and sendid" << e->sendId();

        Q_ASSERT(e->eventType() == QScxmlEvent::ExternalEvent);
        int id = d->m_qStateMachine->postDelayedEvent(e, e->delay());

        qCDebug(scxmlLog) << name() << ": delayed event" << e->name() << "(" << e << ") got id:" << id;
    } else {
        QStateMachine::EventPriority priority =
                e->eventType() == QScxmlEvent::ExternalEvent ? QStateMachine::NormalPriority
                                                             : QStateMachine::HighPriority;

        qCDebug(scxmlLog) << "queueing event" << e->name() << "in" << name();
        if (d->m_qStateMachine->isRunning())
            d->m_qStateMachine->postEvent(e, priority);
        else
            d->m_qStateMachine->queueEvent(e, priority);
    }
}

void QScxmlStateMachine::submitEvent(const QByteArray &event)
{
    QScxmlEvent *e = new QScxmlEvent;
    e->setName(event);
    e->setEventType(QScxmlEvent::ExternalEvent);
    submitEvent(e);
}

void QScxmlStateMachine::submitEvent(const QByteArray &event, const QVariant &data)
{
    QVariant incomingData = data;
    if (incomingData.canConvert<QJSValue>()) {
        incomingData = incomingData.value<QJSValue>().toVariant();
    }

    QScxmlEvent *e = new QScxmlEvent;
    e->setName(event);
    e->setEventType(QScxmlEvent::ExternalEvent);
    e->setData(incomingData);
    submitEvent(e);
}

void QScxmlStateMachine::cancelDelayedEvent(const QByteArray &sendid)
{
    Q_D(QScxmlStateMachine);

    int id = d->m_qStateMachine->eventIdForDelayedEvent(sendid);

    qCDebug(scxmlLog) << name() << ": canceling event" << sendid << "with id" << id;

    if (id != -1)
        d->m_qStateMachine->cancelDelayedEvent(id);
}

void QScxmlInternal::WrappedQStateMachine::queueEvent(QScxmlEvent *event, EventPriority priority)
{
    Q_D(WrappedQStateMachine);

    qCDebug(scxmlLog) << d->m_stateMachine->name() << ": queueing event" << event->name();

    if (!d->m_queuedEvents)
        d->m_queuedEvents = new QVector<WrappedQStateMachinePrivate::QueuedEvent>();
    d->m_queuedEvents->append(WrappedQStateMachinePrivate::QueuedEvent(event, priority));
}

void QScxmlInternal::WrappedQStateMachine::submitQueuedEvents()
{
    Q_D(WrappedQStateMachine);

    qCDebug(scxmlLog) << d->m_stateMachine->name() << ": submitting queued events";

    if (d->m_queuedEvents) {
        foreach (const WrappedQStateMachinePrivate::QueuedEvent &e, *d->m_queuedEvents)
            postEvent(e.event, e.priority);
        delete d->m_queuedEvents;
        d->m_queuedEvents = Q_NULLPTR;
    }
}

int QScxmlInternal::WrappedQStateMachine::eventIdForDelayedEvent(const QByteArray &scxmlEventId)
{
    Q_D(WrappedQStateMachine);
    return d->eventIdForDelayedEvent(scxmlEventId);
}

bool QScxmlStateMachine::isLegalTarget(const QString &target) const
{
    return target.startsWith(QLatin1Char('#'));
}

bool QScxmlStateMachine::isDispatchableTarget(const QString &target) const
{
    Q_D(const QScxmlStateMachine);

    if (isInvoked() && target == QStringLiteral("#_parent"))
        return true; // parent state machine, if we're <invoke>d.
    if (target == QStringLiteral("#_internal")
            || target == QStringLiteral("#_scxml_%1").arg(sessionId()))
        return true; // that's the current state machine

    if (target.startsWith(QStringLiteral("#_"))) {
        QStringRef targetId = target.midRef(2);
        foreach (QScxmlInvokableService *service, d->m_invokedServices) {
            if (service->id() == targetId) {
                return true;
            }
        }
    }

    return false;
}

void QScxmlStateMachine::onFinished()
{
    // The final state is also a stable state.
    emit reachedStableState(true);
}

void QScxmlStateMachine::start()
{
    Q_D(QScxmlStateMachine);

    d->m_qStateMachine->start();
}

QT_END_NAMESPACE
