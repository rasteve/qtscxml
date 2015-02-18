/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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

#include "scxmlparser.h"
#include <QFinalState>
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

static Q_LOGGING_CATEGORY(scxmlParserLog, "scxml.parser")

namespace Scxml {

ScxmlParser::ScxmlParser(QXmlStreamReader *reader, const QString &basedir)
    : m_table(0)
    , m_currentTransition(0)
    , m_currentParent(0)
    , m_currentState(0)
    , m_basedir(basedir)
    , m_reader(reader)
    , m_state(StartingParsing)
{ }

void ScxmlParser::ensureInitialState(const QString &initialId)
{
    if (!initialId.isEmpty()) {
        QAbstractState *initialState = table()->idToValue<QAbstractState>(initialId, true);
        if (initialState) {
            m_currentParent->setInitialState(initialState);
        } else {
            addError(QStringLiteral("could not resolve '%1', for the initial state of %2")
                     .arg(initialId, table()->objectId(m_currentParent)));
            m_state = ParsingError;
            return;
        }
    }
    if (!m_currentParent->initialState()) {
        QAbstractState *firstState = 0;
        loopOnSubStates(m_currentParent, [&firstState](QState *s) -> bool {
            if (!firstState)
                firstState = s;
            return false;
        }, 0, [&firstState](QAbstractState *s) -> void {
            if (!firstState)
                firstState = s;
        });
        if (firstState) {
            qDebug() << "setting initial state of" << table()->objectId(m_currentParent) << " to "
                     << table()->objectId(firstState);
            m_currentParent->setInitialState(firstState);
        }
    }
}

void ScxmlParser::parse()
{
    m_table = new StateTable;
    m_currentParent = m_table;
    m_currentState = m_table;
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
            if (!m_stack.isEmpty() || m_state != FinishedParsing) {
                addError("document finished without a proper scxml item");
                m_state = ParsingError;
            } else {
                m_state = FinishedParsing;
            }
            break;
        case QXmlStreamReader::StartElement:
            // The reader reports the start of an element with namespaceUri() and name(). Empty
            // elements are also reported as StartElement, followed directly by EndElement.
            // The convenience function readElementText() can be called to concatenate all content
            // until the corresponding EndElement. Attributes are reported in attributes(),
            // namespace declarations in namespaceDeclarations().
        {
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
                    Q_ASSERT(0); // to do, use Scxml::XmlNode
                }
                }*/
                break;
            } else if (elName == QLatin1String("scxml")) {
                if (m_state != StartingParsing || !m_stack.isEmpty()) {
                    addError("found scxml tag mid stream");
                    m_state = ParsingError;
                    return;
                }
                if (!checkAttributes(attributes, "version|initial,datamodel,binding,name")) return;
                if (m_reader->namespaceUri() != QLatin1String("http://www.w3.org/2005/07/scxml")) {
                    addError("default namespace must be set with xmlns=\"http://www.w3.org/2005/07/scxml\" in the scxml tag");
                    return;
                }
                if (attributes.value(QLatin1String("version")) != QLatin1String("1.0")) {
                    addError("unsupported scxml version, expected 1.0 in scxml tag");
                    return;
                }
                ParserState pNew = ParserState(ParserState::Scxml);
                pNew.initialId = attributes.value(QLatin1String("initial")).toString();
                QStringRef datamodel = attributes.value(QLatin1String("datamodel"));
                if (datamodel.isEmpty() || datamodel == QLatin1String("null")) {
                    m_table->setDataModel(StateTable::None);
                } else if (datamodel == QLatin1String("ecmascript")) {
                    m_table->setDataModel(StateTable::Javascript);
                } else if (datamodel == QLatin1String("json")) {
                    m_table->setDataModel(StateTable::Json);
                } else if (datamodel == QLatin1String("xpath")) {
                    m_table->setDataModel(StateTable::Xml);
                } else {
                    addError(QStringLiteral("Unsupported data model '%1' in scxml")
                             .arg(datamodel.toString()));
                }
                QStringRef binding = attributes.value(QLatin1String("binding"));
                if (binding.isEmpty() || binding == QLatin1String("early")) {
                    m_table->setDataBinding(StateTable::EarlyBinding);
                } else if (binding == QLatin1String("late")) {
                    m_table->setDataBinding(StateTable::LateBinding);
                } else {
                    addError(QStringLiteral("Unsupperted binding type '%1'")
                             .arg(binding.toString()));
                    return;
                }
                QStringRef name = attributes.value(QLatin1String("name"));
                if (!name.isEmpty())
                    m_table->_name = name.toString();
                m_currentState = m_currentParent = m_table;
                pNew.instructionContainer = &m_table->m_initialSetup;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("state")) {
                if (!checkAttributes(attributes, "|id,initial")) return;
                QState *newState = new ScxmlState(m_currentParent);
                if (!maybeId(attributes, newState)) return;
                ParserState pNew = ParserState(ParserState::State);
                pNew.initialId = attributes.value(QLatin1String("initial")).toString();
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("parallel")) {
                if (!checkAttributes(attributes, "|id")) return;
                QState *newState = new ScxmlState(m_currentParent);
                if (!maybeId(attributes, newState)) return;
                newState->setChildMode(QState::ParallelStates);
                m_currentState = m_currentParent = newState;
                m_stack.append(ParserState(ParserState::Parallel));
            } else if (elName == QLatin1String("initial")) {
                if (!checkAttributes(attributes, "")) return;
                if (m_currentParent->childMode() == QState::ParallelStates) {
                    addError(QStringLiteral("Explicit initial state for parallel states not supported (only implicitly through the initial states of its substates)"));
                    m_state = ParsingError;
                    return;
                }
                ParserState pNew(ParserState::Initial);
                QState *newState = new ScxmlInitialState(m_currentParent);
                m_currentState = m_currentParent = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("transition")) {
                if (!checkAttributes(attributes, "|event,cond,target,type")) return;
                m_currentTransition = new ScxmlTransition(m_currentParent,
                                attributes.value("event").toString(),
                                attributes.value("target").toString().split(QLatin1Char(' ')),
                                attributes.value("cond").toString());
                ParserState pNew = ParserState(ParserState::Transition);
                pNew.instructionContainer = &m_currentTransition->instructionsOnTransition;
                QStringRef type = attributes.value("type");
                if (!type.isEmpty() && type != QLatin1String("internal")) {
                    addError(QStringLiteral("only internal transitions are supported"));
                    m_state = ParsingError;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("final")) {
                if (!checkAttributes(attributes, "|id")) return;
                QFinalState *newState = new QFinalState(m_currentParent);
                if (!maybeId(attributes, newState)) return;
                m_currentState = newState;
                m_stack.append(ParserState(ParserState::Final));
            } else if (elName == QLatin1String("onentry")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnEntry);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                    pNew.instructionContainer = &qobject_cast<ScxmlFinalState *>(m_currentState)->onEntryInstruction;
                    break;
                case ParserState::State:
                case ParserState::Parallel:
                    pNew.instructionContainer = &qobject_cast<ScxmlState *>(m_currentState)->onEntryInstruction;
                    break;
                default:
                    addError("unexpected container state for onentry");
                    m_state = ParsingError;
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("onexit")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew(ParserState::OnExit);
                switch (m_stack.last().kind) {
                case ParserState::Final:
                    pNew.instructionContainer = &qobject_cast<ScxmlFinalState *>(m_currentState)->onExitInstruction;
                    break;
                case ParserState::State:
                case ParserState::Parallel:
                    pNew.instructionContainer = &qobject_cast<ScxmlState *>(m_currentState)->onExitInstruction;
                    break;
                default:
                    addError("unexpected container state for onexit");
                    m_state = ParsingError;
                    break;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("history")) {
                if (!checkAttributes(attributes, "|id,type")) return;
                QHistoryState *newState = new QHistoryState(m_currentParent);
                if (!maybeId(attributes, newState)) return;
                QStringRef type = attributes.value(QLatin1String("type"));
                if (type.isEmpty() || type == QLatin1String("shallow")) {
                    newState->setHistoryType(QHistoryState::ShallowHistory);
                } else if (type == QLatin1String("deep")) {
                    newState->setHistoryType(QHistoryState::DeepHistory);
                } else {
                    addError(QStringLiteral("invalid history type %1, valid values are 'shallow' and 'deep'").arg(type.toString()));
                    m_state = ParsingError;
                    return;
                }
                ParserState pNew = ParserState(ParserState::State);
                m_currentState = newState;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("raise")) {
                if (!checkAttributes(attributes, "event")) return;
                ParserState pNew = ParserState(ParserState::Raise);
                ExecutableContent::Raise *raiseI = new ExecutableContent::Raise(m_currentParent, m_currentTransition);
                raiseI->event = attributes.value(QLatin1String("event")).toString();
                pNew.instruction = raiseI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("if")) {
                if (!checkAttributes(attributes, "cond")) return;
                ParserState pNew = ParserState(ParserState::If);
                ExecutableContent::If *ifI = new ExecutableContent::If(m_currentParent, m_currentTransition);
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                ifI->blocks.append(ExecutableContent::InstructionSequence());
                pNew.instruction = ifI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("elseif")) {
                if (!checkAttributes(attributes, "cond")) return;
                Q_ASSERT(m_stack.last().instruction->instructionKind() == ExecutableContent::Instruction::If);
                ExecutableContent::If *ifI = static_cast<ExecutableContent::If *>(m_stack.last().instruction);
                ifI->conditions.append(attributes.value(QLatin1String("cond")).toString());
                ifI->blocks.append(ExecutableContent::InstructionSequence());
            } else if (elName == QLatin1String("else")) {
                if (!checkAttributes(attributes, "")) return;
                Q_ASSERT(m_stack.last().instruction->instructionKind() == ExecutableContent::Instruction::If);
                ExecutableContent::If *ifI = static_cast<ExecutableContent::If *>(m_stack.last().instruction);
                ifI->blocks.append(ExecutableContent::InstructionSequence());
            } else if (elName == QLatin1String("foreach")) {
                if (!checkAttributes(attributes, "array,item|index")) return;
                ParserState pNew = ParserState(ParserState::Foreach);
                ExecutableContent::Foreach *foreachI = new ExecutableContent::Foreach(m_currentParent, m_currentTransition);
                foreachI->array = attributes.value(QLatin1String("array")).toString();
                foreachI->item = attributes.value(QLatin1String("item")).toString();
                foreachI->index = attributes.value(QLatin1String("index")).toString();
                pNew.instruction = foreachI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("log")) {
                if (!checkAttributes(attributes, "|label,expr")) return;
                ParserState pNew = ParserState(ParserState::Log);
                ExecutableContent::Log *logI = new ExecutableContent::Log(m_currentParent, m_currentTransition);
                logI->label = attributes.value(QLatin1String("label")).toString();
                logI->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = logI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("datamodel")) {
                if (!checkAttributes(attributes, "")) return;
                m_stack.append(ParserState(ParserState::DataModel));
            } else if (elName == QLatin1String("data")) {
                if (!checkAttributes(attributes, "id|src,expr")) return;
                ScxmlData data;
                data.id = attributes.value("id").toString();
                data.src = attributes.value("src").toString();
                data.expr = attributes.value("expr").toString();
                data.context = m_currentParent;
                table()->m_data.append(data);
                m_stack.append(ParserState(ParserState::Data));
            } else if (elName == QLatin1String("assign")) {
                if (!checkAttributes(attributes, "location|expr")) return;
                ParserState pNew = ParserState(ParserState::Assign);
                ExecutableContent::AssignExpression *assign = new ExecutableContent::AssignExpression(m_currentParent, m_currentTransition);
                assign->location = attributes.value(QLatin1String("location")).toString();
                assign->expression = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = assign;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("donedata")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew = ParserState(ParserState::DoneData);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("content")) {
                if (!checkAttributes(attributes, "")) return;
                ParserState pNew = ParserState(ParserState::Content);
                m_stack.append(pNew);
            } else if (elName == QLatin1String("param")) {
                if (!checkAttributes(attributes, "name|expr,location")) return;
                ParserState pNew = ParserState(ParserState::Param);
                ExecutableContent::Param param;
                param.name = attributes.value(QLatin1String("name")).toString();
                param.expr = attributes.value(QLatin1String("expr")).toString();
                param.location = attributes.value(QLatin1String("location")).toString();
                if (m_stack.last().kind == ParserState::DoneData) {
                    static_cast<ScxmlFinalState *>(m_currentState)->doneData.params.append(param);
                } else if (m_stack.last().kind == ParserState::Send) {
                    static_cast<ExecutableContent::Send *>(m_stack.last().instruction)->params.append(param);
                } else if (m_stack.last().kind == ParserState::Invoke) {
                    static_cast<ExecutableContent::Send *>(m_stack.last().instruction)->params.append(param);
                } else {
                    addError(QStringLiteral("unexpected parent of param %1").arg(m_stack.last().kind));
                    m_state = ParsingError;
                }
                m_stack.append(pNew);
            } else if (elName == QLatin1String("script")) {
                if (!checkAttributes(attributes, "|src")) return;
                ParserState pNew = ParserState(ParserState::Script);
                ExecutableContent::JavaScript *script = new ExecutableContent::JavaScript(m_currentParent, m_currentTransition);
                script->src = attributes.value(QLatin1String("src")).toString();
                pNew.instruction = script;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("send")) {
                if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr,target,targetexpr")) return;
                ParserState pNew = ParserState(ParserState::Send);
                ExecutableContent::Send *send = new ExecutableContent::Send(m_currentParent, m_currentTransition);
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
                send->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '));
                send->content = 0;
                pNew.instruction = send;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("cancel")) {
                if (!checkAttributes(attributes, "|sendid,sendidexpr")) return;
                ParserState pNew = ParserState(ParserState::Cancel);
                ExecutableContent::Cancel *cancel = new ExecutableContent::Cancel(m_currentParent, m_currentTransition);
                cancel->sendid = attributes.value(QLatin1String("sendid")).toString();
                cancel->sendidexpr = attributes.value(QLatin1String("sendidexpr")).toString();
                pNew.instruction = cancel;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("invoke")) {
                if (!checkAttributes(attributes, "|event,eventexpr,id,idlocation,type,typeexpr,namelist,delay,delayexpr")) return;
                ParserState pNew = ParserState(ParserState::Invoke);
                ExecutableContent::Invoke *invoke = new ExecutableContent::Invoke(m_currentParent, m_currentTransition);
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
                invoke->namelist = attributes.value(QLatin1String("namelist")).toString().split(QLatin1Char(' '));
                pNew.instruction = invoke;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("finalize")) {
                ParserState pNew(ParserState::Finalize);
                Q_ASSERT(m_stack.last().instruction->instructionKind() == ExecutableContent::Instruction::Invoke);
                pNew.instructionContainer = &static_cast<ExecutableContent::Invoke *>(m_stack.last().instruction)->finalize;
                m_stack.append(pNew);
            } else {
                qCWarning(scxmlParserLog) << "unexpected element " << elName;
            }
            if (m_stack.size()>1 && !m_stack.at(m_stack.size()-2).validChild(m_stack.last().kind)) {
                addError("invalid child ");
                m_state = ParsingError;
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
                ensureInitialState(p.initialId);
                m_state = FinishedParsing;
                return;
            case ParserState::State:
                ensureInitialState(p.initialId);
                m_currentState = m_currentParent = m_currentParent->parentState();
                break;
            case ParserState::Parallel:
                if (!p.initialId.isEmpty()) {
                    addError(QStringLiteral("initial states (like '%1'), not supported for parallel state %2")
                             .arg(p.initialId, table()->objectId(m_currentParent)));
                    m_state = ParsingError;
                    return;
                }
                m_currentState = m_currentParent = m_currentParent->parentState();
                break;
            case ParserState::Initial: {
                if (m_currentParent->transitions().size() != 1) {
                    addError("initial state should have exactly one transition");
                    m_state = ParsingError;
                    return;
                }
                ScxmlTransition *t = qobject_cast<ScxmlTransition *>(m_currentParent->transitions().first());
                if (!t->eventSelector.isEmpty()
                        || !t->conditionalExp.isEmpty()) {
                    addError("transition in initial state should have no event or condition");
                    m_state = ParsingError;
                    return;
                }
                QState *parentParent = m_currentParent->parentState();
                if (!t->instructionsOnTransition.statements.isEmpty() || t->targetIds().size() > 1) {
                    qDebug() << "setting initial state using substate and eventless transition";
                    if (!parentParent || parentParent->childMode() == QState::ParallelStates) {
                        addError("initial state of parallel states not supported");
                        m_state = ParsingError;
                        return;
                    }
                    parentParent->setInitialState(m_currentParent);
                } else {
                    if (!t->targetIds().isEmpty()) // else error?
                        m_stack.last().initialId = t->targetIds().first();
                    delete m_currentParent;
                }
                m_currentState = m_currentParent = parentParent;
                break;
            }
            case ParserState::Final:
            case ParserState::History:
                m_currentState = m_currentParent;
                break;
            case ParserState::Transition:
                Q_ASSERT(m_currentTransition);
                m_currentParent->addTransition(m_currentTransition);
                m_currentTransition = 0;
            case ParserState::OnEntry:
                break;
            case ParserState::OnExit:
                break;
            case ParserState::ElseIf:
            case ParserState::Else:
                break;
            case ParserState::Raise:
            case ParserState::If:
            case ParserState::Foreach:
            case ParserState::Log:
            case ParserState::Assign:
            case ParserState::Script:
            case ParserState::Send:
            case ParserState::Cancel:
            case ParserState::Invoke: {
                ExecutableContent::InstructionSequence *instructions = m_stack.last().instructionContainer;
                if (!instructions) {
                    addError("got executable content within an element that did not set insttuctionContainer");
                    m_state = ParsingError;
                    return;
                }
                instructions->statements.append(ExecutableContent::Instruction::Ptr(p.instruction));
                p.instruction = 0;
                break;
            }
            case ParserState::Finalize:
            case ParserState::DataModel:
            case ParserState::Data:
            case ParserState::DataElement:
            case ParserState::DoneData:
            case ParserState::Content:
            case ParserState::Param:
            case ParserState::None:
                break;
            }
        }
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
        addError("Error parsing scxml file");
        addError(m_reader->errorString());
        m_state = ParsingError;
    }
}

void ScxmlParser::addError(const QString &msg, ErrorMessage::Severity severity)
{
    m_errors.append(ErrorMessage(severity, msg, QStringLiteral("%1:%2 %3").arg(m_reader->lineNumber())
                                 .arg(m_reader->columnNumber())
                                 .arg((m_reader->error() != QXmlStreamReader::NoError) ? m_reader->errorString() : QString())));
    switch (severity){
    case ErrorMessage::Debug:
        qCDebug(scxmlLog) << m_errors.last().msg << m_errors.last().parserState;
        break;
    case ErrorMessage::Info:
        qCWarning(scxmlLog) << m_errors.last().msg << m_errors.last().parserState;
        break;
    case ErrorMessage::Error:
        qCWarning(scxmlLog) << m_errors.last().msg << m_errors.last().parserState;
        break;
    }
    if (severity == ErrorMessage::Error)
        m_state = ParsingError;
}

void ScxmlParser::addError(const char *msg, ErrorMessage::Severity severity)
{
    addError(QString::fromLatin1(msg), severity);
}

bool ScxmlParser::maybeId(const QXmlStreamAttributes &attributes, QObject *obj)
{
    if (!obj) {
        addError("Null object in maybeId");
        m_state = ParsingError;
        return false;
    }
    QStringRef idStr = attributes.value(QLatin1String("id"));
    if (!idStr.isEmpty()) {
        obj->setObjectName(idStr.toString());
        if (!m_table->addId(obj->objectName(), obj, errorDumper())) {
            m_state = ParsingError;
            return false;
        }
    }
    return true;
}

bool ScxmlParser::checkAttributes(const QXmlStreamAttributes &attributes, const char *attribStr)
{
    QString allAttrib = QString::fromLatin1(attribStr);
    QStringList attrSplit = allAttrib.split(QLatin1Char('|'));
    QStringList requiredNames, optionalNames;
    requiredNames = attrSplit.value(0).split(QLatin1Char(','), QString::SkipEmptyParts);
    optionalNames = attrSplit.value(1).split(QLatin1Char(','), QString::SkipEmptyParts);
    if (attrSplit.size() > 2) {
        addError("Internal error, invalid attribStr in checkAttributes");
        m_state = ParsingError;
    }
    foreach (const QString &rName, requiredNames)
        if (rName.isEmpty())
            requiredNames.removeOne(rName);
    foreach (const QString &oName, optionalNames)
        if (oName.isEmpty())
            optionalNames.removeOne(oName);
    return checkAttributes(attributes, requiredNames, optionalNames);
}

bool ScxmlParser::checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames, QStringList optionalNames)
{
    foreach (const QXmlStreamAttribute &attribute, attributes) {
        QStringRef ns = attribute.namespaceUri();
        if (!ns.isEmpty() && ns != QLatin1String("http://www.w3.org/2005/07/scxml")) {
            foreach (const QString &nsToIgnore, m_namespacesToIgnore) {
                if (ns == nsToIgnore)
                    continue;
            }
            m_namespacesToIgnore << ns.toString();
            addError(QStringLiteral("Ignoring unexpected namespace %1").arg(ns.toString()),
                     ErrorMessage::Info);
            continue;
        }
        const QString name = attribute.name().toString();
        if (!requiredNames.removeOne(name) && !optionalNames.contains(name)) {
            addError(QStringLiteral("Unexpected attribute '%1'").arg(name));
            m_state = ParsingError;
            return false;
        }
    }
    if (!requiredNames.isEmpty()) {
        addError(QStringLiteral("Missing required attributes: '%1'")
                 .arg(requiredNames.join(QLatin1String("', '"))));
        m_state = ParsingError;
        return false;
    }
    return true;
}

bool Scxml::ParserState::collectChars() {
    switch (kind) {
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
        return false;
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
    case ParserState::Content:

    case ParserState::Param:
    case ParserState::Script:
    case ParserState::Send:
    case ParserState::Cancel:
    case ParserState::Invoke:
    case ParserState::Finalize:
        return isExecutableContent(child);
        break;
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

} // namespace QScxml
