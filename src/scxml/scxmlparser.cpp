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

#include "scxmlparser_p.h"
#include "scxmlstatemachine_p.h"
#include "executablecontent_p.h"
#include "nulldatamodel.h"
#include "ecmascriptdatamodel.h"
#include "scxmlqstates.h"
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QState>
#include <QHistoryState>
#include <QEventTransition>
#include <QSignalTransition>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVector>
#include <private/qabstracttransition_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <QJSValue>

namespace {
enum {
    DebugHelper_NameTransitions = 0
};
} // anonymous namespace

QT_BEGIN_NAMESPACE
namespace Scxml {

static QString scxmlNamespace = QStringLiteral("http://www.w3.org/2005/07/scxml");
static QString qtScxmlNamespace = QStringLiteral("http://theqtcompany.com/scxml/2015/06/");

class ScxmlVerifier: public DocumentModel::NodeVisitor
{
public:
    ScxmlVerifier(std::function<void (const DocumentModel::XmlLocation &, const QString &)> errorHandler)
        : m_errorHandler(errorHandler)
        , m_doc(Q_NULLPTR)
        , m_hasErrors(false)
    {}

    bool verify(DocumentModel::ScxmlDocument *doc)
    {
        if (doc->isVerified)
            return true;

        doc->isVerified = true;
        m_doc = doc;
        foreach (DocumentModel::AbstractState *state, doc->allStates) {
            if (state->id.isEmpty()) {
                continue;
#ifndef QT_NO_DEBUG
            } else if (m_stateById.contains(state->id)) {
                Q_ASSERT(!"Should be unreachable: the parser should check for this case!");
#endif // QT_NO_DEBUG
            } else {
                m_stateById[state->id] = state;
            }
        }

        doc->root->accept(this);
        return !m_hasErrors;
    }

private:
    bool visit(DocumentModel::Scxml *scxml) Q_DECL_OVERRIDE
    {
        Q_ASSERT(scxml->initialStates.isEmpty());

        if (scxml->initial.isEmpty()) {
            if (auto firstChild = firstAbstractState(scxml)) {
                scxml->initialStates.append(firstChild);
            }
        } else {
            foreach (const QString &initial, scxml->initial) {
                if (DocumentModel::AbstractState *s = m_stateById.value(initial))
                    scxml->initialStates.append(s);
                else
                    error(scxml->xmlLocation, QStringLiteral("initial state '%1' not found for <scxml> element").arg(initial));
            }
        }

        m_parentNodes.append(scxml);

        return true;
    }

    void endVisit(DocumentModel::Scxml *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::State *state) Q_DECL_OVERRIDE
    {
        Q_ASSERT(state->initialStates.isEmpty());

        if (state->initial.isEmpty()) {
            if (auto firstChild = firstAbstractState(state)) {
                state->initialStates += firstChild;
            }
        } else {
            Q_ASSERT(state->type == DocumentModel::State::Normal);
            foreach (const QString &initialState, state->initial) {
                if (DocumentModel::AbstractState *s = m_stateById.value(initialState)) {
                    state->initialStates += s;
                } else {
                    error(state->xmlLocation,
                          QStringLiteral("undefined initial state '%1' for state '%2'")
                            .arg(initialState, state->id));
                }
            }
        }

        switch (state->type) {
        case DocumentModel::State::Normal:
            break;
        case DocumentModel::State::Parallel:
            if (!state->initial.isEmpty()) {
                error(state->xmlLocation, QStringLiteral("parallel states cannot have an initial state"));
            }
            break;
        case DocumentModel::State::Initial:
            if (transitionCount(state) != 1)
                error(state->xmlLocation, QStringLiteral("an initial state can only have one transition, but has '%1'").arg(transitionCount(state)));
            if (DocumentModel::Transition *t = firstTransition(state)) {
                if (!t->events.isEmpty() || !t->condition.isNull()) {
                    error(t->xmlLocation, QStringLiteral("the transition in an initial state cannot have an event or a condition"));
                }
                if (t->targets.isEmpty()) {
                    error(t->xmlLocation, QStringLiteral("the transition in an initial state must have at least one target"));
                }
            }
            foreach (DocumentModel::StateOrTransition *child, state->children) {
                if (DocumentModel::State *s = child->asState()) {
                    error(s->xmlLocation, QStringLiteral("substates are not allowed in initial states"));
                }
            }
            if (parentState() == Q_NULLPTR) {
                error(state->xmlLocation, QStringLiteral("initial states can only occur in a state"));
            }
            break;
        case DocumentModel::State::Final:
            break;
        default:
            Q_UNREACHABLE();
        }

        m_parentNodes.append(state);
        return true;
    }

    void endVisit(DocumentModel::State *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::Transition *transition) Q_DECL_OVERRIDE
    {
        Q_ASSERT(transition->targetStates.isEmpty());

        if (int size = transition->targets.size())
            transition->targetStates.reserve(size);
        foreach (const QString &target, transition->targets) {
            if (DocumentModel::AbstractState *s = m_stateById.value(target)) {
                if (transition->targetStates.contains(s)) {
                    error(transition->xmlLocation, QStringLiteral("duplicate target '%1'").arg(target));
                } else {
                    transition->targetStates.append(s);
                }
            } else if (!target.isEmpty()) {
                error(transition->xmlLocation, QStringLiteral("unknown state '%1' in target").arg(target));
            }
        }

        m_parentNodes.append(transition);
        return true;
    }

    void endVisit(DocumentModel::Transition *) Q_DECL_OVERRIDE
    {
        m_parentNodes.removeLast();
    }

    bool visit(DocumentModel::HistoryState *state) Q_DECL_OVERRIDE
    {
        bool seenTransition = false;
        foreach (DocumentModel::StateOrTransition *sot, state->children) {
            if (DocumentModel::State *s = sot->asState()) {
                error(s->xmlLocation, QStringLiteral("history state cannot have substates"));
            } else if (DocumentModel::Transition *t = sot->asTransition()) {
                if (seenTransition) {
                    error(t->xmlLocation, QStringLiteral("history state can only have one transition"));
                } else {
                    seenTransition = true;
                    m_parentNodes.append(state);
                    t->accept(this);
                    m_parentNodes.removeLast();
                }
            }
        }

        return false;
    }

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("send"), QStringLiteral("eventexpr"), node->eventexpr);
        return true;
    }

    void visit(DocumentModel::Cancel *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("cancel"), QStringLiteral("sendidexpr"), node->sendidexpr);
    }

    bool visit(DocumentModel::DoneData *node) Q_DECL_OVERRIDE
    {
        checkExpr(node->xmlLocation, QStringLiteral("donedata"), QStringLiteral("expr"), node->expr);
        return false;
    }

    bool visit(DocumentModel::Invoke *node) Q_DECL_OVERRIDE
    {
        ScxmlVerifier subVerifier(m_errorHandler);
        m_hasErrors = !subVerifier.verify(node->content.data());
        return false;
    }

private:
    static int transitionCount(DocumentModel::State *state)
    {
        int count = 0;
        foreach (DocumentModel::StateOrTransition *child, state->children) {
            if (child->asTransition())
                ++count;
        }
        return count;
    }

    static DocumentModel::Transition *firstTransition(DocumentModel::State *state)
    {
        foreach (DocumentModel::StateOrTransition *child, state->children) {
            if (DocumentModel::Transition *t = child->asTransition())
                return t;
        }
        return Q_NULLPTR;
    }

    static DocumentModel::AbstractState *firstAbstractState(DocumentModel::StateContainer *container)
    {
        QVector<DocumentModel::StateOrTransition *> children;
        if (auto state = container->asState())
            children = state->children;
        else if (auto scxml = container->asScxml())
            children = scxml->children;
        else
            Q_UNREACHABLE();
        foreach (DocumentModel::StateOrTransition *child, children) {
            if (DocumentModel::State *s = child->asState())
                return s;
            else if (DocumentModel::HistoryState *h = child->asHistoryState())
                return h;
        }
        return Q_NULLPTR;
    }

    void checkExpr(const DocumentModel::XmlLocation &loc, const QString &tag, const QString &attrName, const QString &attrValue)
    {
        if (m_doc->root->dataModel == DocumentModel::Scxml::NullDataModel && !attrValue.isEmpty()) {
            error(loc, QStringLiteral("%1 in <%2> cannot be used with data model 'null'").arg(attrName, tag));
        }
    }

    void error(const DocumentModel::XmlLocation &location, const QString &message)
    {
        m_hasErrors = true;
        if (m_errorHandler)
            m_errorHandler(location, message);
    }

    DocumentModel::Node *parentState() const
    {
        for (int i = m_parentNodes.size() - 1; i >= 0; --i) {
            if (DocumentModel::State *s = m_parentNodes.at(i)->asState())
                return s;
        }

        return Q_NULLPTR;
    }

private:
    std::function<void (const DocumentModel::XmlLocation &, const QString &)> m_errorHandler;
    DocumentModel::ScxmlDocument *m_doc;
    bool m_hasErrors;
    QHash<QString, DocumentModel::AbstractState *> m_stateById;
    QVector<DocumentModel::Node *> m_parentNodes;
};

class QStateMachineBuilder;
class DynamicStateMachine: public StateMachine, public Scxml::ScxmlEventFilter
{
    // Manually expanded from Q_OBJECT macro:
public:
    Q_OBJECT_CHECK

    const QMetaObject *metaObject() const Q_DECL_OVERRIDE
    { return m_metaObject; }

    int qt_metacall(QMetaObject::Call _c, int _id, void **_a) Q_DECL_OVERRIDE
    {
        _id = StateMachine::qt_metacall(_c, _id, _a);
        if (_id < 0)
            return _id;
        int ownMethodCount = m_metaObject->methodCount() - m_metaObject->methodOffset();
        if (_c == QMetaObject::InvokeMetaMethod) {
            if (_id < ownMethodCount)
                qt_static_metacall(this, _c, _id, _a);
            _id -= ownMethodCount;
        } else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
                   || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
            qt_static_metacall(this, _c, _id, _a);
            _id -= m_metaObject->propertyCount();
        }
        return _id;
    }

private:
    Q_DECL_HIDDEN static void qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
    {
        if (_c == QMetaObject::InvokeMetaMethod) {
            DynamicStateMachine *_t = static_cast<DynamicStateMachine *>(_o);
            if (_id >= _t->m_eventNamesByIndex.size() || _id < 0) {
                Q_UNREACHABLE();
            }
            const QByteArray &event = _t->m_eventNamesByIndex.at(_id);
            if (!event.isEmpty()) {
                if (_id < _t->m_firstSlotWithoutData) {
                    QVariant data = *reinterpret_cast< QVariant(*)>(_a[1]);
                    if (data.canConvert<QJSValue>()) {
                        data = data.value<QJSValue>().toVariant();
                    }
                    _t->submitEvent(event, data);
                } else {
                    _t->submitEvent(event, QVariant());
                }
            }
        } else if (_c == QMetaObject::RegisterPropertyMetaType) {
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType<QAbstractState *>();
        } else if (_c == QMetaObject::ReadProperty) {
            DynamicStateMachine *_t = static_cast<DynamicStateMachine *>(_o);
            void *_v = _a[0];
            if (_id >= 0 && _id < _t->m_propertyNamesByIndex.size()) {
                *reinterpret_cast<QAbstractState**>(_v) = StateMachinePrivate::get(_t)->stateByScxmlName(_t->m_propertyNamesByIndex.at(_id));
            }
        }
    }
    // end of Q_OBJECT macro

private:
    friend QStateMachineBuilder;
    DynamicStateMachine()
        : m_metaObject(Q_NULLPTR)
        , m_firstSlot(0)
        , m_firstSlotWithoutData(0)
    {
        // Temporarily wire up the QMetaObject, because qobject_cast needs it while building MyQStateMachine.
        QMetaObjectBuilder b;
        b.setClassName("DynamicStateMachine");
        b.setSuperClass(&StateMachine::staticMetaObject);
        b.setStaticMetacallFunction(qt_static_metacall);
        m_metaObject = b.toMetaObject();

        setScxmlEventFilter(this);
    }

    void initDynamicParts(const QSet<QByteArray> &eventSignals,
                          const QSet<QByteArray> &eventSlots,
                          const QSet<QString> &stateNames)
    {
        // Release the temporary QMetaObject.
        Q_ASSERT(m_metaObject);
        free(m_metaObject);

        m_eventNamesByIndex.reserve(eventSignals.size() + eventSlots.size());

        // Build the real one.
        QMetaObjectBuilder b;
        b.setClassName("DynamicStateMachine");
        b.setSuperClass(&StateMachine::staticMetaObject);
        b.setStaticMetacallFunction(qt_static_metacall);

        // signals
        foreach (const QByteArray &eventName, eventSignals) {
            QByteArray signalName = QByteArray("event_") + eventName + "(const QVariant &)";
            QMetaMethodBuilder signalBuilder = b.addSignal(signalName);
            signalBuilder.setParameterNames({"data"});
            int idx = signalBuilder.index();
            m_eventNamesByIndex.resize(std::max(idx + 1, m_eventNamesByIndex.size()));
            m_eventNamesByIndex[idx] = eventName;
        }

        // slots
        m_firstSlot = m_eventNamesByIndex.size();
        foreach (const QByteArray &eventName, eventSlots) {
            QByteArray slotName = QByteArray("event_") + eventName + "(const QVariant &)";
            QMetaMethodBuilder slotBuilder = b.addSlot(slotName);
            slotBuilder.setParameterNames({"data"});
            int idx = slotBuilder.index();
            m_eventNamesByIndex.resize(std::max(idx + 1, m_eventNamesByIndex.size()));
            m_eventNamesByIndex[idx] = eventName;
        }

        m_firstSlotWithoutData = m_eventNamesByIndex.size();
        foreach (const QByteArray &eventName, eventSlots) {
            QByteArray slotName = QByteArray("event_") + eventName + "()";
            QMetaMethodBuilder slotBuilder = b.addSlot(slotName);
            int idx = slotBuilder.index();
            m_eventNamesByIndex.resize(std::max(idx + 1, m_eventNamesByIndex.size()));
            m_eventNamesByIndex[idx] = eventName;
        }

        // properties
        foreach (const QString &stateName, stateNames) {
            QMetaPropertyBuilder prop = b.addProperty(stateName.toUtf8(), "QAbstractState *");
            prop.setWritable(false);
            prop.setConstant(true);
            int idx = prop.index();
            m_propertyNamesByIndex.resize(std::max(idx + 1, m_propertyNamesByIndex.size()));
            m_propertyNamesByIndex[idx] = stateName;
        }

        m_metaObject = b.toMetaObject();
    }

public:
    ~DynamicStateMachine()
    { if (m_metaObject) free(m_metaObject); }

    bool handle(QScxmlEvent *event, Scxml::StateMachine *stateMachine) Q_DECL_OVERRIDE {
        Q_UNUSED(stateMachine);

        if (event->originType() != QStringLiteral("qt:signal")) {
            return true;
        }

        auto eventName = event->name();
        for (int i = 0; i < m_firstSlot; ++i) {
            if (m_eventNamesByIndex.at(i) == eventName) {
                QVariant data = event->data();
                void *argv[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&data)) };
                QMetaObject::activate(this, metaObject(), i, argv);
                return false;
            }
        }

        return true;
    }

private:
    QMetaObject *m_metaObject;
    QVector<QByteArray> m_eventNamesByIndex;
    QVector<QString> m_propertyNamesByIndex;
    int m_firstSlot;
    int m_firstSlotWithoutData;
};

class InvokeDynamicScxmlFactory: public ScxmlInvokableServiceFactory
{
public:
    InvokeDynamicScxmlFactory(ExecutableContent::StringId invokeLocation,
                       ExecutableContent::StringId id,
                       ExecutableContent::StringId idPrefix,
                       ExecutableContent::StringId idlocation,
                       const QVector<ExecutableContent::StringId> &namelist,
                       bool autoforward,
                       const QVector<Param> &params)
        : ScxmlInvokableServiceFactory(invokeLocation, id, idPrefix, idlocation, namelist, autoforward, params)
    {}

    void setContent(const QSharedPointer<DocumentModel::ScxmlDocument> &content)
    { m_content = content; }

    ScxmlInvokableService *invoke(StateMachine *child) Q_DECL_OVERRIDE;

private:
    QSharedPointer<DocumentModel::ScxmlDocument> m_content;
};

class QStateMachineBuilder: public ExecutableContent::Builder
{
public:
    QStateMachineBuilder()
        : m_table(Q_NULLPTR)
        , m_currentTransition(Q_NULLPTR)
        , m_bindLate(false)
    {}

    StateMachine *build(DocumentModel::ScxmlDocument *doc)
    {
        m_table = Q_NULLPTR;
        m_parents.reserve(32);
        m_allTransitions.reserve(doc->allTransitions.size());
        m_docStatesToQStates.reserve(doc->allStates.size());

        doc->root->accept(this);
        wireTransitions();
        applyInitialStates();

        ExecutableContent::DynamicTableData *td = tableData();
        td->setParent(m_table);
        m_table->setTableData(td);
        m_table->initDynamicParts(m_eventSignals, m_eventSlots, m_stateNames);

        m_parents.clear();
        m_allTransitions.clear();
        m_docStatesToQStates.clear();
        m_currentTransition = Q_NULLPTR;

        return m_table;
    }

private:
    using NodeVisitor::visit;
    using ExecutableContent::Builder::createContext;

    bool visit(DocumentModel::Scxml *node) Q_DECL_OVERRIDE
    {
        m_table = new DynamicStateMachine;

        switch (node->binding) {
        case DocumentModel::Scxml::EarlyBinding:
            m_table->setDataBinding(StateMachine::EarlyBinding);
            break;
        case DocumentModel::Scxml::LateBinding:
            m_table->setDataBinding(StateMachine::LateBinding);
            m_bindLate = true;
            break;
        default:
            Q_UNREACHABLE();
        }

        setName(node->name);

        m_parents.append(StateMachinePrivate::get(m_table)->m_qStateMachine);
        visit(node->children);

        m_dataElements.append(node->dataElements);
        if (node->script || !m_dataElements.isEmpty() || !node->initialSetup.isEmpty()) {
            setInitialSetup(startNewSequence());
            generate(m_dataElements);
            if (node->script) {
                node->script->accept(this);
            }
            visit(&node->initialSetup);
            endSequence();
        }

        m_parents.removeLast();

        foreach (auto initialState, node->initialStates) {
            Q_ASSERT(initialState);
            m_initialStates.append(qMakePair(StateMachinePrivate::get(m_table)->m_qStateMachine, initialState));
        }

        return false;
    }

    bool visit(DocumentModel::State *node) Q_DECL_OVERRIDE
    {
        QAbstractState *newState = Q_NULLPTR;
        switch (node->type) {
        case DocumentModel::State::Normal: {
            auto s = new ScxmlState(currentParent());
            newState = s;
            foreach (DocumentModel::AbstractState *initialState, node->initialStates) {
                m_initialStates.append(qMakePair(s, initialState));
            }
        } break;
        case DocumentModel::State::Parallel: {
            auto s = new ScxmlState(currentParent());
            s->setChildMode(QState::ParallelStates);
            newState = s;
        } break;
        case DocumentModel::State::Initial: {
            auto s = new ScxmlState(currentParent());
            currentParent()->setInitialState(s);
            newState = s;
        } break;
        case DocumentModel::State::Final: {
            auto s = new ScxmlFinalState(currentParent());
            newState = s;
            s->setDoneData(generate(node->doneData));
        } break;
        default:
            Q_UNREACHABLE();
        }

        newState->setObjectName(node->id);
        m_stateNames.insert(node->id);

        m_docStatesToQStates.insert(node, newState);
        m_parents.append(newState);

        if (!node->dataElements.isEmpty()) {
            if (m_bindLate) {
                qobject_cast<ScxmlState *>(newState)->setInitInstructions(startNewSequence());
                generate(node->dataElements);
                endSequence();
            } else {
                m_dataElements.append(node->dataElements);
            }
        }

        ExecutableContent::ContainerId onEntry = generate(node->onEntry);
        ExecutableContent::ContainerId onExit = generate(node->onExit);
        if (ScxmlState *s = qobject_cast<ScxmlState *>(newState)) {
            s->setOnEntryInstructions(onEntry);
            s->setOnExitInstructions(onExit);
            if (!node->invokes.isEmpty()) {
                QVector<ScxmlInvokableServiceFactory *> factories;
                foreach (DocumentModel::Invoke *invoke, node->invokes) {
                    auto ctxt = createContext(QStringLiteral("invoke"));
                    QVector<ExecutableContent::StringId> namelist;
                    foreach (const QString &name, invoke->namelist)
                        namelist += addString(name);
                    QVector<ScxmlInvokableServiceFactory::Param> params;
                    foreach (DocumentModel::Param *param, invoke->params) {
                        params.append({
                                          addString(param->name),
                                          createEvaluatorVariant(QStringLiteral("param"), QStringLiteral("expr"), param->expr),
                                          addString(param->location)
                                      });
                    }
                    auto factory = new InvokeDynamicScxmlFactory(ctxt,
                                                                 addString(invoke->id),
                                                                 addString(node->id + QStringLiteral(".session-")),
                                                                 addString(invoke->idLocation),
                                                                 namelist,
                                                                 invoke->autoforward,
                                                                 params);
                    factory->setContent(invoke->content);
                    factories.append(factory);
                }
                s->setInvokableServiceFactories(factories);
            }
        } else if (ScxmlFinalState *f = qobject_cast<ScxmlFinalState *>(newState)) {
            f->setOnEntryInstructions(onEntry);
            f->setOnExitInstructions(onExit);
        } else {
            Q_UNREACHABLE();
        }

        visit(node->children);

        m_parents.removeLast();
        return false;
    }

    bool visit(DocumentModel::Transition *node) Q_DECL_OVERRIDE
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        auto events = toUtf8(node->events);
        m_eventSlots.unite(events.toSet());
        auto newTransition = new ScxmlTransition(events);
        if (QHistoryState *parent = qobject_cast<QHistoryState*>(m_parents.last())) {
            parent->setDefaultTransition(newTransition);
        } else {
            currentParent()->addTransition(newTransition);
        }
#else // See QTBUG-46703, which explains why the following is a bad work-around.
        QState *parentState = Q_NULLPTR;
        if (QHistoryState *parent = qobject_cast<QHistoryState*>(m_parents.last())) {
            // QHistoryState cannot have an initial transition, only an initial state.
            // So, work around that by creating an initial state, and add the transition to that.
            parentState = new ScxmlState(parent->parentState());
            parent->setDefaultState(parentState);
        } else {
            parentState = currentParent();
        }
        auto newTransition = new ScxmlTransition(parentState, toUtf8(node->events));
        parentState->addTransition(newTransition);
#endif

        if (node->condition) {
            auto cond = createEvaluatorBool(QStringLiteral("transition"), QStringLiteral("cond"), *node->condition.data());
            newTransition->setConditionalExpression(cond);
        }

        switch (node->type) {
        case DocumentModel::Transition::External:
            newTransition->setTransitionType(QAbstractTransition::ExternalTransition);
            break;
        case DocumentModel::Transition::Internal:
            newTransition->setTransitionType(QAbstractTransition::InternalTransition);
            break;
        default:
            Q_UNREACHABLE();
        }

        m_allTransitions.insert(newTransition, node);
        if (!node->instructionsOnTransition.isEmpty()) {
            m_currentTransition = newTransition;
            newTransition->setInstructionsOnTransition(startNewSequence());
            visit(&node->instructionsOnTransition);
            endSequence();
            m_currentTransition = 0;
        }
        Q_ASSERT(newTransition->stateMachine());
        return false;
    }

    bool visit(DocumentModel::HistoryState *state) Q_DECL_OVERRIDE
    {
        QHistoryState *newState = new QHistoryState(currentParent());
        switch (state->type) {
        case DocumentModel::HistoryState::Shallow:
            newState->setHistoryType(QHistoryState::ShallowHistory);
            break;
        case DocumentModel::HistoryState::Deep:
            newState->setHistoryType(QHistoryState::DeepHistory);
            break;
        default:
            Q_UNREACHABLE();
        }

        newState->setObjectName(state->id);
        m_docStatesToQStates.insert(state, newState);
        m_parents.append(newState);
        return true;
    }

    void endVisit(DocumentModel::HistoryState *) Q_DECL_OVERRIDE
    {
        m_parents.removeLast();
    }

    bool visit(DocumentModel::Send *node) Q_DECL_OVERRIDE
    {
        if (node->type == QStringLiteral("qt:signal")) {
            m_eventSignals.insert(node->event.toUtf8());
        }

        return ExecutableContent::Builder::visit(node);
    }

private: // Utility methods
    static QList<QByteArray> toUtf8(const QStringList &l)
    {
        QList<QByteArray> res;
        foreach (const QString &s, l)
            res.append(s.toUtf8());
        return res;
    }

    QState *currentParent() const
    {
        if (m_parents.isEmpty())
            return Q_NULLPTR;

        QState *parent = qobject_cast<QState*>(m_parents.last());
        Q_ASSERT(parent);
        return parent;
    }

    void wireTransitions()
    {
        for (QHash<QAbstractTransition *, DocumentModel::Transition*>::const_iterator i = m_allTransitions.begin(), ei = m_allTransitions.end(); i != ei; ++i) {
            QList<QAbstractState *> targets;
            targets.reserve(i.value()->targets.size());
            foreach (DocumentModel::AbstractState *targetState, i.value()->targetStates) {
                QAbstractState *target = m_docStatesToQStates.value(targetState);
                Q_ASSERT(target);
                targets.append(target);
            }
            i.key()->setTargetStates(targets);

            if (DebugHelper_NameTransitions)
                i.key()->setObjectName(QStringLiteral("%1 -> %2").arg(i.key()->parent()->objectName(), i.value()->targets.join(QStringLiteral(","))));
        }
    }

    void applyInitialStates()
    {
        foreach (const auto &init, m_initialStates) {
            Q_ASSERT(init.second);
            auto initialState = m_docStatesToQStates.value(init.second);
            Q_ASSERT(initialState);
            init.first->setInitialState(initialState);
        }
    }

    QString createContextString(const QString &instrName) const Q_DECL_OVERRIDE
    {
        if (m_currentTransition) {
            QString state;
            if (QState *s = m_currentTransition->sourceState()) {
                state = QStringLiteral(" of state '%1'").arg(s->objectName());
            }
            return QStringLiteral("%1 instruction in transition %2 %3").arg(instrName, m_currentTransition->objectName(), state);
        } else {
            return QStringLiteral("%1 instruction in state %2").arg(instrName, m_parents.last()->objectName());
        }
    }

    QString createContext(const QString &instrName, const QString &attrName, const QString &attrValue) const Q_DECL_OVERRIDE
    {
        QString location = createContextString(instrName);
        return QStringLiteral("%1 with %2=\"%3\"").arg(location, attrName, attrValue);
    }

    DataModel *dataModel() const
    { return m_table->dataModel(); }

private:
    DynamicStateMachine *m_table;
    QVector<QAbstractState *> m_parents;
    QHash<QAbstractTransition *, DocumentModel::Transition*> m_allTransitions;
    QHash<DocumentModel::AbstractState *, QAbstractState *> m_docStatesToQStates;
    QAbstractTransition *m_currentTransition;
    QVector<QPair<QState *, DocumentModel::AbstractState *>> m_initialStates;
    bool m_bindLate;
    QVector<DocumentModel::DataElement *> m_dataElements;
    QSet<QByteArray> m_eventSignals;
    QSet<QByteArray> m_eventSlots;
    QSet<QString> m_stateNames;
};

inline ScxmlInvokableService *InvokeDynamicScxmlFactory::invoke(StateMachine *parent)
{
    auto child = QStateMachineBuilder().build(m_content.data());

    //-----
    // FIXME: duplicated code from ScxmlParser::instantiateDataModel
    DataModel *dataModel = Q_NULLPTR;
    switch (m_content->root->dataModel) {
    case DocumentModel::Scxml::NullDataModel:
        dataModel = new NullDataModel;
        break;
    case DocumentModel::Scxml::JSDataModel:
        dataModel = new EcmaScriptDataModel;
        break;
    default:
        Q_UNREACHABLE();
    }
    child->setDataModel(dataModel);
    StateMachinePrivate::get(child)->parserData()->m_ownedDataModel.reset(dataModel);
    //-----

    return finishInvoke(child, parent);
}

ScxmlParser::ScxmlParser(QXmlStreamReader *reader)
    : p(new ScxmlParserPrivate(this, reader))
{ }

ScxmlParser::~ScxmlParser()
{
    delete p;
}

QString ScxmlParser::fileName() const
{
    return p->fileName();
}

void ScxmlParser::setFileName(const QString &fileName)
{
    p->setFilename(fileName);
}

void ScxmlParser::parse()
{
    p->parse();
}

StateMachine *ScxmlParser::instantiateStateMachine() const
{
    if (DocumentModel::ScxmlDocument *doc = p->scxmlDocument()) {
        return QStateMachineBuilder().build(doc);
    } else {
        auto table = new StateMachine;
        StateMachinePrivate::get(table)->parserData()->m_errors = errors();
        return table;
    }
}

void ScxmlParser::instantiateDataModel(StateMachine *table) const
{
    DataModel *dataModel = Q_NULLPTR;
    switch (p->scxmlDocument()->root->dataModel) {
    case DocumentModel::Scxml::NullDataModel:
        dataModel = new NullDataModel;
        break;
    case DocumentModel::Scxml::JSDataModel:
        dataModel = new EcmaScriptDataModel;
        break;
    default:
        Q_UNREACHABLE();
    }
    table->setDataModel(dataModel);
    StateMachinePrivate::get(table)->parserData()->m_ownedDataModel.reset(dataModel);
}

ScxmlParser::State ScxmlParser::state() const
{
    return p->state();
}

QVector<Scxml::ScxmlError> ScxmlParser::errors() const
{
    return p->errors();
}

void ScxmlParser::addError(const QString &msg)
{
    p->addError(msg);
}

bool Scxml::ParserState::collectChars() {
    switch (kind) {
    case Content:
    case Data:
    case Script:
        return true;
    default:
        break;
    }
    return false;
}

bool ParserState::validChild(ParserState::Kind child) const {
    return validChild(kind, child);
}

bool ParserState::validChild(ParserState::Kind parent, ParserState::Kind child)
{
    switch (parent) {
    case ParserState::Scxml:
        switch (child) {
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::Final:
        case ParserState::DataModel:
        case ParserState::Script:
        case ParserState::Transition:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::State:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::Transition:
        case ParserState::Initial:
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::Final:
        case ParserState::History:
        case ParserState::DataModel:
        case ParserState::Invoke:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::Parallel:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::Transition:
        case ParserState::State:
        case ParserState::Parallel:
        case ParserState::History:
        case ParserState::DataModel:
        case ParserState::Invoke:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::Transition:
        return isExecutableContent(child);
    case ParserState::Initial:
        return (child == ParserState::Transition);
    case ParserState::Final:
        switch (child) {
        case ParserState::OnEntry:
        case ParserState::OnExit:
        case ParserState::DoneData:
            return true;
        default:
            break;
        }
        return false;
    case ParserState::OnEntry:
    case ParserState::OnExit:
        return isExecutableContent(child);
        return false;
    case ParserState::History:
        return (child == ParserState::Transition);
    case ParserState::Raise:
        return false;
    case ParserState::If:
        return (child == ParserState::ElseIf || child == ParserState::Else
                || isExecutableContent(child));
    case ParserState::ElseIf:
    case ParserState::Else:
        return false;
    case ParserState::Foreach:
        return isExecutableContent(child);
    case ParserState::Log:
        return false;
    case ParserState::DataModel:
        return (child == ParserState::Data);
    case ParserState::Data:
        return (child == ParserState::DataElement);
    case ParserState::DataElement:
        return (child == ParserState::DataElement);
    case ParserState::Assign:
        return (child == ParserState::DataElement);
    case ParserState::DoneData:
        return (child == ParserState::Content || child == ParserState::Param);
    case ParserState::Send:
        return (child == ParserState::Param || child == ParserState::Content
                || isExecutableContent(child));
    case ParserState::Content:
    case ParserState::Param:
    case ParserState::Cancel:
    case ParserState::Finalize:
        return isExecutableContent(child);
        break;
    case ParserState::Invoke:
        return child == ParserState::Content;
    case ParserState::Script:
    case ParserState::None:
        break;
    }
    return false;
}

bool Scxml::ParserState::isExecutableContent(Scxml::ParserState::Kind kind) {
    switch (kind) {
    case Raise:
    case Send:
    case Log:
    case Script:
    case Assign:
    case If:
    case Foreach:
    case Cancel:
    case Invoke:
        return true;
    default:
        break;
    }
    return false;
}

DocumentModel::Node::~Node()
{
}

void DocumentModel::DataElement::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Param::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::DoneData::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        foreach (Param *param, params)
            param->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Send::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(params);
    }
    visitor->endVisit(this);
}

void DocumentModel::Invoke::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(params);
        visitor->visit(&finalize);
    }
    visitor->endVisit(this);
}

void DocumentModel::Raise::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Log::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Script::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::Assign::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::If::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(blocks);
    }
    visitor->endVisit(this);
}

void DocumentModel::Foreach::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(&block);
    }
    visitor->endVisit(this);
}

void DocumentModel::Cancel::accept(DocumentModel::NodeVisitor *visitor)
{
    visitor->visit(this);
}

void DocumentModel::State::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(dataElements);
        visitor->visit(children);
        visitor->visit(onEntry);
        visitor->visit(onExit);
        if (doneData)
            doneData->accept(visitor);
        foreach (Invoke *invoke, invokes)
            invoke->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Transition::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(&instructionsOnTransition);
    }
    visitor->endVisit(this);
}

void DocumentModel::HistoryState::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        if (Transition *t = defaultConfiguration())
            t->accept(visitor);
    }
    visitor->endVisit(this);
}

void DocumentModel::Scxml::accept(DocumentModel::NodeVisitor *visitor)
{
    if (visitor->visit(this)) {
        visitor->visit(children);
        visitor->visit(dataElements);
        if (script)
            script->accept(visitor);
        visitor->visit(&initialSetup);
    }
    visitor->endVisit(this);
}

DocumentModel::NodeVisitor::~NodeVisitor()
{}

ScxmlParser::Loader::Loader(ScxmlParser *parser)
    : m_parser(parser)
{}

ScxmlParser::Loader::~Loader()
{}

ScxmlParser *ScxmlParser::Loader::parser() const
{
    return m_parser;
}

QByteArray DefaultLoader::load(const QString &name, const QString &baseDir, bool *ok)
{
    Q_ASSERT(ok != nullptr);

    *ok = false;
    QFileInfo fInfo(name);
    if (fInfo.isRelative())
        fInfo = QFileInfo(QDir(baseDir).filePath(name));
    if (!fInfo.exists()) {
        parser()->addError(QStringLiteral("src attribute resolves to non existing file (%1)").arg(fInfo.absoluteFilePath()));
    } else {
        QFile f(fInfo.absoluteFilePath());
        if (f.open(QFile::ReadOnly)) {
            *ok = true;
            return f.readAll();
        } else {
            parser()->addError(QStringLiteral("Failure opening file %1: %2")
                               .arg(fInfo.absoluteFilePath(), f.errorString()));
        }
    }
    return QByteArray();
}

ScxmlParserPrivate *ScxmlParserPrivate::get(ScxmlParser *parser)
{
    return parser->p;
}

ScxmlParserPrivate::ScxmlParserPrivate(ScxmlParser *parser, QXmlStreamReader *reader)
    : m_parser(parser)
    , m_currentParent(Q_NULLPTR)
    , m_currentState(Q_NULLPTR)
    , m_defaultLoader(parser)
    , m_loader(&m_defaultLoader)
    , m_reader(reader)
    , m_state(ScxmlParser::StartingParsing)
{}

ScxmlParser *ScxmlParserPrivate::parser() const
{
    return m_parser;
}

DocumentModel::ScxmlDocument *ScxmlParserPrivate::scxmlDocument()
{
    if (!m_doc)
        return Q_NULLPTR;

    auto handler = [this](const DocumentModel::XmlLocation &location, const QString &msg) {
        this->addError(location, msg);
    };

    if (ScxmlVerifier(handler).verify(m_doc.data()))
        return m_doc.data();
    else
        return Q_NULLPTR;
}

QString ScxmlParserPrivate::fileName() const
{
    return m_fileName;
}

void ScxmlParserPrivate::setFilename(const QString &fileName)
{
    m_fileName = fileName;
}

void ScxmlParserPrivate::parse()
{
    m_doc.reset(new DocumentModel::ScxmlDocument(fileName()));
    m_currentParent = m_doc->root;
    m_currentState = m_doc->root;
    while (!m_reader->atEnd()) {
        QXmlStreamReader::TokenType tt = m_reader->readNext();
        switch (tt) {
        case QXmlStreamReader::NoToken:
            // The reader has not yet read anything.
            continue;
        case QXmlStreamReader::Invalid:
            // An error has occurred, reported in error() and errorString().
            break;
        case QXmlStreamReader::StartDocument:
            // The reader reports the XML version number in documentVersion(), and the encoding
            // as specified in the XML document in documentEncoding(). If the document is declared
            // standalone, isStandaloneDocument() returns true; otherwise it returns false.
            break;
        case QXmlStreamReader::EndDocument:
            // The reader reports the end of the document.
            if (!m_stack.isEmpty() || m_state != ScxmlParser::FinishedParsing) {
                addError(QStringLiteral("document finished without a proper scxml item"));
            }
            break;
        case QXmlStreamReader::StartElement:
            // The reader reports the start of an element with namespaceUri() and name(). Empty
            // elements are also reported as StartElement, followed directly by EndElement.
            // The convenience function readElementText() can be called to concatenate all content
            // until the corresponding EndElement. Attributes are reported in attributes(),
            // namespace declarations in namespaceDeclarations().
        {
            auto ns = m_reader->namespaceUri();
            if (ns != scxmlNamespace) {
                m_reader->skipCurrentElement();
                continue;
            }

            QStringRef elName = m_reader->name();
            QXmlStreamAttributes attributes = m_reader->attributes();
            if (!m_stack.isEmpty() && (m_stack.last().kind == ParserState::DataElement
                                       || m_stack.last().kind == ParserState::Data)) {
                /*switch (m_table->dataModel()) {
                case StateTable::None:
                    break; // error?
                case StateTable::Json:
                case StateTable::Javascript:
                {
                    ParserState pNew = ParserState(ParserState::DataElement);
                    QJsonObject obj;
                    foreach (const QXmlStreamAttribute &attribute, attributes)
                        obj.insert(QStringLiteral("@").append(attribute.name()), attribute.value().toString());
                    pNew.jsonValue = obj;
                    m_stack.append(pNew);
                    break;
                }
                case StateTable::Xml:
                {
                    ParserState pNew = ParserState(ParserState::DataElement);
                    Q_ASSERT(0);
                }
                }*/
                break;
            } else if (elName == QLatin1String("scxml")) {
                m_doc->root = new DocumentModel::Scxml(xmlLocation());
                m_doc->root->xmlLocation = xmlLocation();
                auto scxml = m_doc->root;
                if (m_state != ScxmlParser::StartingParsing || !m_stack.isEmpty()) {
                    addError(xmlLocation(), QStringLiteral("found scxml tag mid stream"));
                    return;
                } else {
                    m_state = ScxmlParser::ParsingScxml;
                }
                if (!checkAttributes(attributes, "version|initial,datamodel,binding,name,classname")) return;
                if (m_reader->namespaceUri() != scxmlNamespace) {
                    addError(QStringLiteral("default namespace must be set with xmlns=\"%1\" in the scxml tag").arg(scxmlNamespace));
                    return;
                }
                if (attributes.value(QLatin1String("version")) != QLatin1String("1.0")) {
                    addError(QStringLiteral("unsupported scxml version, expected 1.0 in scxml tag"));
                    return;
                }
                ParserState pNew = ParserState(ParserState::Scxml);
                if (attributes.hasAttribute(QStringLiteral("initial"))) {
                    QString initial = attributes.value(QStringLiteral("initial")).toString();
                    scxml->initial += initial.split(QChar::Space, QString::SkipEmptyParts);
                }
                QStringRef datamodel = attributes.value(QLatin1String("datamodel"));
                if (datamodel.isEmpty() || datamodel == QLatin1String("null")) {
                    scxml->dataModel = DocumentModel::Scxml::NullDataModel;
                } else if (datamodel == QLatin1String("ecmascript")) {
                    scxml->dataModel = DocumentModel::Scxml::JSDataModel;
                } else {
                    addError(QStringLiteral("Unsupported data model '%1' in scxml")
                             .arg(datamodel.toString()));
                }
                QStringRef binding = attributes.value(QLatin1String("binding"));
                if (binding.isEmpty() || binding == QLatin1String("early")) {
                    scxml->binding = DocumentModel::Scxml::EarlyBinding;
                } else if (binding == QLatin1String("late")) {
                    scxml->binding = DocumentModel::Scxml::LateBinding;
                } else {
                    addError(QStringLiteral("Unsupperted binding type '%1'")
                             .arg(binding.toString()));
                    return;
                }
                QStringRef name = attributes.value(QLatin1String("name"));
                if (!name.isEmpty()) {
                    scxml->name = name.toString();
                }
                QStringRef qtClassname = attributes.value(qtScxmlNamespace, QStringLiteral("classname"));
                if (!qtClassname.isEmpty()) {
                    scxml->qtClassname = qtClassname.toString();
                }
                m_currentState = m_currentParent = m_doc->root;
                pNew.instructionContainer = &m_doc->root->initialSetup;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("state")) {
                if (!checkAttributes(attributes, "|id,initial")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Normal, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                ParserState pNew = ParserState(ParserState::State);
                if (attributes.hasAttribute(QStringLiteral("initial"))) {
                    QString initial = attributes.value(QStringLiteral("initial")).toString();
                    newState->initial += initial.split(QChar::Space, QString::SkipEmptyParts);
                }
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("parallel")) {
                if (!checkAttributes(attributes, "|id")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Parallel, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                m_currentState = m_currentParent = newState;
                m_stack.append(ParserState(ParserState::Parallel));
            } else if (elName == QLatin1String("initial")) {
                if (!checkAttributes(attributes, "")) return;
                if (currentParent()->asState()->type == DocumentModel::State::Parallel) {
                    addError(QStringLiteral("Explicit initial state for parallel states not supported (only implicitly through the initial states of its substates)"));
                    return;
                }
                ParserState pNew(ParserState::Initial);
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Initial, xmlLocation());
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("transition")) {
                if (!checkAttributes(attributes, "|event,cond,target,type")) return;
                auto transition = m_doc->newTransition(m_currentParent, xmlLocation());
                transition->events = attributes.value(QLatin1String("event")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                transition->targets = attributes.value(QLatin1String("target")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                if (attributes.hasAttribute(QStringLiteral("cond")))
                    transition->condition.reset(new QString(attributes.value(QLatin1String("cond")).toString()));
                QStringRef type = attributes.value(QLatin1String("type"));
                if (type.isEmpty() || type == QLatin1String("external")) {
                    transition->type = DocumentModel::Transition::External;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
                } else if (type == QLatin1String("internal")) {
                    transition->type = DocumentModel::Transition::Internal;
#endif
                } else {
                    addError(QStringLiteral("invalid transition type '%1', valid values are 'external' and 'internal'").arg(type.toString()));
                    break;
                }
                ParserState pNew = ParserState(ParserState::Transition);
                pNew.instructionContainer = &transition->instructionsOnTransition;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("final")) {
                if (!checkAttributes(attributes, "|id")) return;
                auto newState = m_doc->newState(m_currentParent, DocumentModel::State::Final, xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                m_currentState = m_currentParent = newState;
                m_stack.append(ParserState(ParserState::Final));
            } else if (elName == QLatin1String("history")) {
                if (!checkAttributes(attributes, "|id,type")) return;
                auto newState = m_doc->newHistoryState(currentParent(), xmlLocation());
                if (!maybeId(attributes, &newState->id)) return;
                QStringRef type = attributes.value(QLatin1String("type"));
                if (type.isEmpty() || type == QLatin1String("shallow")) {
                    newState->type = DocumentModel::HistoryState::Shallow;
                } else if (type == QLatin1String("deep")) {
                    newState->type = DocumentModel::HistoryState::Deep;
                } else {
                    addError(QStringLiteral("invalid history type %1, valid values are 'shallow' and 'deep'").arg(type.toString()));
                    return;
                }
                ParserState pNew = ParserState(ParserState::History);
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("onentry")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnEntry);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                case ParserState::State:
                case ParserState::Parallel:
                    if (DocumentModel::State *s = m_currentState->asState()) {
                        pNew.instructionContainer = m_doc->newSequence(&s->onEntry);
                        break;
                    }
                    // intentional fall-through
                default:
                    addError(QStringLiteral("unexpected container state for onentry"));
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("onexit")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnExit);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                case ParserState::State:
                case ParserState::Parallel:
                    if (DocumentModel::State *s = m_currentState->asState()) {
                        pNew.instructionContainer = m_doc->newSequence(&s->onExit);
                        break;
                    }
                    // intentional fall-through
                default:
                    addError(QStringLiteral("unexpected container state for onexit"));
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("raise")) {
                if (!checkAttributes(attributes, "event")) return;
                ParserState pNew = ParserState(ParserState::Raise);
                auto raise = m_doc->newNode<DocumentModel::Raise>(xmlLocation());
                raise->event = attributes.value(QLatin1String("event")).toString();
                pNew.instruction = raise;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("if")) {
                if (!checkAttributes(attributes, "cond")) return;
                ParserState pNew = ParserState(ParserState::If);
                auto *ifI = m_doc->newNode<DocumentModel::If>(xmlLocation());
                pNew.instruction = ifI;
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                pNew.instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("elseif")) {
                if (!checkAttributes(attributes, "cond")) return;
                DocumentModel::If *ifI = m_stack.last().instruction->asIf();
                Q_ASSERT(ifI);
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                m_stack.last().instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(ParserState(ParserState::ElseIf));
            } else if (elName == QLatin1String("else")) {
                if (!checkAttributes(attributes, "")) return;
                DocumentModel::If *ifI = m_stack.last().instruction->asIf();
                Q_ASSERT(ifI);
                m_stack.last().instructionContainer = m_doc->newSequence(&ifI->blocks);
                m_stack.append(ParserState(ParserState::Else));
            } else if (elName == QLatin1String("foreach")) {
                if (!checkAttributes(attributes, "array,item|index")) return;
                ParserState pNew = ParserState(ParserState::Foreach);
                auto foreachI = m_doc->newNode<DocumentModel::Foreach>(xmlLocation());
                foreachI->array = attributes.value(QLatin1String("array")).toString();
                foreachI->item = attributes.value(QLatin1String("item")).toString();
                foreachI->index = attributes.value(QLatin1String("index")).toString();
                pNew.instruction = foreachI;
                pNew.instructionContainer = &foreachI->block;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("log")) {
                if (!checkAttributes(attributes, "|label,expr")) return;
                ParserState pNew = ParserState(ParserState::Log);
                auto logI = m_doc->newNode<DocumentModel::Log>(xmlLocation());
                logI->label = attributes.value(QLatin1String("label")).toString();
                logI->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = logI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("datamodel")) {
                if (!checkAttributes(attributes, "")) return;
                m_stack.append(ParserState(ParserState::DataModel));
            } else if (elName == QLatin1String("data")) {
                if (!checkAttributes(attributes, "id|src,expr")) return;
                auto data = m_doc->newNode<DocumentModel::DataElement>(xmlLocation());
                data->id = attributes.value(QLatin1String("id")).toString();
                data->src = attributes.value(QLatin1String("src")).toString();
                data->expr = attributes.value(QLatin1String("expr")).toString();
                if (!data->src.isEmpty()) {
                    addError(QStringLiteral("the source attribute in a data tag is unsupported")); // FIXME: use a loader like in <script>
                }
                if (DocumentModel::Scxml *scxml = m_currentParent->asScxml()) {
                    scxml->dataElements.append(data);
                } else if (DocumentModel::State *state = m_currentParent->asState()) {
                    state->dataElements.append(data);
                } else {
                    Q_UNREACHABLE();
                }
                m_stack.append(ParserState(ParserState::Data));
            } else if (elName == QLatin1String("assign")) {
                if (!checkAttributes(attributes, "location|expr")) return;
                ParserState pNew = ParserState(ParserState::Assign);
                auto assign = m_doc->newNode<DocumentModel::Assign>(xmlLocation());
                assign->location = attributes.value(QLatin1String("location")).toString();
                assign->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = assign;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("donedata")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew = ParserState(ParserState::DoneData);
                m_stack.append(pNew);
                bool handled = false;
                if (DocumentModel::State *s = m_currentState->asState()) {
                    if (s->type == DocumentModel::State::Final) {
                        handled = true;
                        if (s->doneData) {
                            addError(QLatin1String("state can only have one donedata"));
                        } else {
                            s->doneData = m_doc->newNode<DocumentModel::DoneData>(xmlLocation());
                        }
                    }
                }
                if (!handled) {
                    addError(QStringLiteral("donedata can only occur in a final state"));
                }
            } else if (elName == QLatin1String("content")) {
                if (!checkAttributes(attributes, "|expr")) return;
                switch (m_stack.last().kind) {
                case ParserState::DoneData: {
                    DocumentModel::State *s = m_currentState->asState();
                    Q_ASSERT(s);
                    s->doneData->expr = attributes.value(QLatin1String("expr")).toString();
                } break;
                case ParserState::Send: {
                    DocumentModel::Send *s = m_stack.last().instruction->asSend();
                    Q_ASSERT(s);
                    s->content = attributes.value(QLatin1String("expr")).toString();
                } break;
                case ParserState::Invoke: {
                    DocumentModel::Invoke *i = m_stack.last().instruction->asInvoke();
                    Q_ASSERT(i);
                    if (attributes.hasAttribute(QStringLiteral("expr"))) {
                        addError(QStringLiteral("expr attribute in content of invoke is not supported"));
                        break;
                    }
                    ScxmlParser p(m_reader);
                    p.setFileName(m_fileName);
                    p.parse();
                    i->content.reset(p.p->m_doc.take());
                    m_errors.append(p.errors());
                    if (p.state() == ScxmlParser::ParsingError)
                        m_state = ScxmlParser::ParsingError;
                } break;
                default:
                    addError(QStringLiteral("unexpected parent of content %1").arg(m_stack.last().kind));
                }
                ParserState pNew = ParserState(ParserState::Content);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("param")) {
                if (!checkAttributes(attributes, "name|expr,location")) return;
                ParserState pNew = ParserState(ParserState::Param);
                auto param = m_doc->newNode<DocumentModel::Param>(xmlLocation());
                param->name = attributes.value(QLatin1String("name")).toString();
                param->expr = attributes.value(QLatin1String("expr")).toString();
                param->location = attributes.value(QLatin1String("location")).toString();
                switch (m_stack.last().kind) {
                case ParserState::DoneData: {
                    DocumentModel::State *s = m_currentState->asState();
                    Q_ASSERT(s);
                    Q_ASSERT(s->doneData);
                    s->doneData->params.append(param);
                } break;
                case ParserState::Send: {
                    DocumentModel::Send *s = m_stack.last().instruction->asSend();
                    Q_ASSERT(s);
                    s->params.append(param);
                } break;
                case ParserState::Invoke: {
                    DocumentModel::Invoke *i = m_stack.last().instruction->asInvoke();
                    Q_ASSERT(i);
                    i->params.append(param);
                } break;
                default:
                    addError(QStringLiteral("unexpected parent of param %1").arg(m_stack.last().kind));
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("script")) {
                if (!checkAttributes(attributes, "|src")) return;
                ParserState pNew = ParserState(ParserState::Script);
                auto *script = m_doc->newNode<DocumentModel::Script>(xmlLocation());
                script->src = attributes.value(QLatin1String("src")).toString();
                pNew.instruction = script;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("send")) {
                if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr,target,targetexpr")) return;
                ParserState pNew = ParserState(ParserState::Send);
                auto *send = m_doc->newNode<DocumentModel::Send>(xmlLocation());
                send->event = attributes.value(QLatin1String("event")).toString();
                send->eventexpr = attributes.value(QLatin1String("eventexpr")).toString();
                send->delay = attributes.value(QLatin1String("delay")).toString();
                send->delayexpr = attributes.value(QLatin1String("delayexpr")).toString();
                send->id = attributes.value(QLatin1String("id")).toString();
                send->idLocation = attributes.value(QLatin1String("idlocation")).toString();
                send->type = attributes.value(QLatin1String("type")).toString();
                send->typeexpr = attributes.value(QLatin1String("typeexpr")).toString();
                send->target = attributes.value(QLatin1String("target")).toString();
                send->targetexpr = attributes.value(QLatin1String("targetexpr")).toString();
                if (attributes.hasAttribute(QLatin1String("namelist")))
                    send->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                pNew.instruction = send;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("cancel")) {
                if (!checkAttributes(attributes, "|sendid,sendidexpr")) return;
                ParserState pNew = ParserState(ParserState::Cancel);
                auto *cancel = m_doc->newNode<DocumentModel::Cancel>(xmlLocation());
                cancel->sendid = attributes.value(QLatin1String("sendid")).toString();
                cancel->sendidexpr = attributes.value(QLatin1String("sendidexpr")).toString();
                pNew.instruction = cancel;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("invoke")) {
                if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr")) return;
                ParserState pNew = ParserState(ParserState::Invoke);
                auto *invoke = m_doc->newNode<DocumentModel::Invoke>(xmlLocation());
                DocumentModel::State *parentState = m_currentParent->asState();
                if (!parentState ||
                        (parentState->type != DocumentModel::State::Normal && parentState->type != DocumentModel::State::Parallel)) {
                    addError(QStringLiteral("invoke can only occur in <state> or <parallel>"));
                    break;
                }
                parentState->invokes.append(invoke);
                invoke->src = attributes.value(QLatin1String("src")).toString();
                invoke->srcexpr = attributes.value(QLatin1String("srcexpr")).toString();
                invoke->id = attributes.value(QLatin1String("id")).toString();
                invoke->idLocation = attributes.value(QLatin1String("idlocation")).toString();
                invoke->type = attributes.value(QLatin1String("type")).toString();
                invoke->typeexpr = attributes.value(QLatin1String("typeexpr")).toString();
                QStringRef autoforwardS = attributes.value(QLatin1String("autoforward"));
                if (QStringRef::compare(autoforwardS, QLatin1String("true"), Qt::CaseInsensitive) == 0
                        || QStringRef::compare(autoforwardS, QLatin1String("yes"), Qt::CaseInsensitive) == 0
                        || QStringRef::compare(autoforwardS, QLatin1String("t"), Qt::CaseInsensitive) == 0
                        || QStringRef::compare(autoforwardS, QLatin1String("y"), Qt::CaseInsensitive) == 0
                        || autoforwardS == QLatin1String("1"))
                    invoke->autoforward = true;
                else
                    invoke->autoforward = false;
                invoke->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
                pNew.instruction = invoke;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("finalize")) {
                ParserState pNew(ParserState::Finalize);
                auto invoke = m_stack.last().instruction->asInvoke();
                Q_ASSERT(invoke);
                pNew.instructionContainer = &invoke->finalize;
                m_stack.append(pNew);
            } else {
                addError(QStringLiteral("unexpected element %1").arg(elName.toString()));
            }
            if (m_stack.size()>1 && !m_stack.at(m_stack.size()-2).validChild(m_stack.last().kind)) {
                addError(QStringLiteral("invalid child"));
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            // The reader reports the end of an element with namespaceUri() and name().
        {
            ParserState p = m_stack.last();
            m_stack.removeLast();
            switch (p.kind) {
            case ParserState::Scxml:
                if (m_state == ScxmlParser::ParsingScxml) {
                    m_state = ScxmlParser::FinishedParsing;
                } else {
                    m_state = ScxmlParser::ParsingError;
                }
                return;
            case ParserState::State:
            case ParserState::Parallel:
            case ParserState::Initial:
            case ParserState::Final:
            case ParserState::History:
                Q_ASSERT(m_currentParent->parent);
                m_currentState = m_currentParent = m_currentParent->parent;
                break;
            case ParserState::Invoke:
            case ParserState::Transition:
            case ParserState::OnEntry:
            case ParserState::OnExit:
            case ParserState::ElseIf:
            case ParserState::Else:
                break;
            case ParserState::Script:
            {
                DocumentModel::Script *scriptI = p.instruction->asScript();
                if (!p.chars.trimmed().isEmpty()) {
                    scriptI->content = p.chars.trimmed();
                    if (!scriptI->src.isEmpty())
                        addError(QStringLiteral("both scr and source content given to script, will ignore external content"));
                } else if (!scriptI->src.isEmpty()) {
                    if (!m_loader) {
                        addError(QStringLiteral("cannot parse a document with external dependencies without a loader"));
                    } else {
                        bool ok;
                        QByteArray data = load(scriptI->src, &ok);
                        if (!ok) {
                            addError(QStringLiteral("failed to load external dependency"));
                        } else {
                            scriptI->content = QString::fromUtf8(data);
                        }
                    }
                }
            } // very intentional fallthrough!
            case ParserState::Raise:
            case ParserState::If:
            case ParserState::Foreach:
            case ParserState::Log:
            case ParserState::Assign:
            case ParserState::Send:
            case ParserState::Cancel: {
                DocumentModel::InstructionSequence *instructions = m_stack.last().instructionContainer;
                if (!instructions) {
                    addError(QStringLiteral("got executable content within an element that did not set instructionContainer"));
                    return;
                }
                instructions->append(p.instruction);
                p.instruction = 0;
                break;
            }
            case ParserState::Finalize:
            case ParserState::DataModel:
            case ParserState::DataElement:
            case ParserState::DoneData:
            case ParserState::Param:
            case ParserState::None:
                break;
            case ParserState::Content:
                if (!p.chars.trimmed().isEmpty()) {
                    Q_ASSERT(!m_stack.isEmpty());
                    switch (m_stack.last().kind) {
                    case ParserState::DoneData: // see test529
                        m_currentState->asState()->doneData->contents = p.chars.simplified();
                        break;
                    case ParserState::Send: // see test179
                        m_stack.last().instruction->asSend()->content = p.chars.simplified();
                        break;
                    default:
                        break;
                    }
                }
                break;
            case ParserState::Data: {
                DocumentModel::DataElement *data = Q_NULLPTR;
                if (auto state = m_currentParent->asState()) {
                    data = state->dataElements.last();
                } else if (auto scxml = m_currentParent->asNode()->asScxml()) {
                    data = scxml->dataElements.last();
                } else {
                    Q_UNREACHABLE();
                }
                if (!data->src.isEmpty() && !data->expr.isEmpty()) {
                    addError(QStringLiteral("data element with both 'src' and 'expr' attributes"));
                    return;
                }
                if (!p.chars.trimmed().isEmpty()) {
                    if (!data->src.isEmpty()) {
                        addError(QStringLiteral("data element with both 'src' attribute and CDATA"));
                        return;
                    } else if (!data->expr.isEmpty()) {
                        addError(QStringLiteral("data element with both 'expr' attribute and CDATA"));
                        return;
                    } else {
                        data->expr = p.chars.simplified();
                    }
                }
            } break;
            } // parser state
        } // QXmlStreamReader::EndElement
        case QXmlStreamReader::Characters:
            // The reader reports characters in text(). If the characters are all white-space,
            // isWhitespace() returns true. If the characters stem from a CDATA section,
            // isCDATA() returns true.
            if (m_stack.isEmpty())
                break;
            if (m_stack.last().collectChars())
                m_stack.last().chars.append(m_reader->text());
            break;
        case QXmlStreamReader::Comment:
            // The reader reports a comment in text().
            break;
        case QXmlStreamReader::DTD:
            // The reader reports a DTD in text(), notation declarations in notationDeclarations(),
            // and entity declarations in entityDeclarations(). Details of the DTD declaration are
            // reported in in dtdName(), dtdPublicId(), and dtdSystemId().
            break;
        case QXmlStreamReader::EntityReference:
            // The reader reports an entity reference that could not be resolved. The name of
            // the reference is reported in name(), the replacement text in text().
            break;
        case QXmlStreamReader::ProcessingInstruction:
            break;
        }
    }
    if (m_reader->hasError()
            && m_reader->error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        addError(QStringLiteral("Error parsing scxml file: %1").arg(m_reader->errorString()));
    }
}

QByteArray ScxmlParserPrivate::load(const QString &name, bool *ok) const
{
    return m_loader->load(name, QFileInfo(m_fileName).absolutePath(), ok);
}

ScxmlParser::State ScxmlParserPrivate::state() const
{
    return m_state;
}

QVector<ScxmlError> ScxmlParserPrivate::errors() const
{
    return m_errors;
}

void ScxmlParserPrivate::addError(const QString &msg)
{
    m_errors.append(ScxmlError(m_fileName, m_reader->lineNumber(), m_reader->columnNumber(), msg));
    m_state = ScxmlParser::ParsingError;
}

void ScxmlParserPrivate::addError(const DocumentModel::XmlLocation &location, const QString &msg)
{
    m_errors.append(ScxmlError(m_fileName, location.line, location.column, msg));
    m_state = ScxmlParser::ParsingError;
}

DocumentModel::AbstractState *ScxmlParserPrivate::currentParent() const
{
    DocumentModel::AbstractState *parent = m_currentParent->asAbstractState();
    Q_ASSERT(!m_currentParent || parent);
    return parent;
}

DocumentModel::XmlLocation ScxmlParserPrivate::xmlLocation() const
{
    return DocumentModel::XmlLocation(m_reader->lineNumber(), m_reader->columnNumber());
}

bool ScxmlParserPrivate::maybeId(const QXmlStreamAttributes &attributes, QString *id)
{
    Q_ASSERT(id);
    QString idStr = attributes.value(QLatin1String("id")).toString();
    if (!idStr.isEmpty()) {
        if (m_allIds.contains(idStr)) {
            addError(xmlLocation(), QStringLiteral("duplicate id '%1'").arg(idStr));
        } else {
            m_allIds.insert(idStr);
            *id = idStr;
        }
    }
    return true;
}

bool ScxmlParserPrivate::checkAttributes(const QXmlStreamAttributes &attributes, const char *attribStr)
{
    QString allAttrib = QString::fromLatin1(attribStr);
    QStringList attrSplit = allAttrib.split(QLatin1Char('|'));
    QStringList requiredNames, optionalNames;
    requiredNames = attrSplit.value(0).split(QLatin1Char(','), QString::SkipEmptyParts);
    optionalNames = attrSplit.value(1).split(QLatin1Char(','), QString::SkipEmptyParts);
    if (attrSplit.size() > 2) {
        addError(QStringLiteral("Internal error, invalid attribStr in checkAttributes"));
    }
    foreach (const QString &rName, requiredNames)
        if (rName.isEmpty())
            requiredNames.removeOne(rName);
    foreach (const QString &oName, optionalNames)
        if (oName.isEmpty())
            optionalNames.removeOne(oName);
    return checkAttributes(attributes, requiredNames, optionalNames);
}

bool ScxmlParserPrivate::checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames, QStringList optionalNames)
{
    foreach (const QXmlStreamAttribute &attribute, attributes) {
        QStringRef ns = attribute.namespaceUri();
        if (!ns.isEmpty() && ns != scxmlNamespace && ns != qtScxmlNamespace) {
            foreach (const QString &nsToIgnore, m_namespacesToIgnore) {
                if (ns == nsToIgnore)
                    continue;
            }
            m_namespacesToIgnore << ns.toString();
            continue;
        }
        const QString name = attribute.name().toString();
        if (!requiredNames.removeOne(name) && !optionalNames.contains(name)) {
            addError(QStringLiteral("Unexpected attribute '%1'").arg(name));
            return false;
        }
    }
    if (!requiredNames.isEmpty()) {
        addError(QStringLiteral("Missing required attributes: '%1'")
                 .arg(requiredNames.join(QLatin1String("', '"))));
        return false;
    }
    return true;
}

} // namespace Scxml
QT_END_NAMESPACE
