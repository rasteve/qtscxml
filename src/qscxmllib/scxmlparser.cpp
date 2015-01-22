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

static Q_LOGGING_CATEGORY(scxmlParserLog, "scxml.parser")

namespace Scxml {

ScxmlParser::ScxmlParser(QIODevice *device) :
    m_reader(new QXmlStreamReader(device)), m_state(StartingParsing) { }

void ScxmlParser::parse()
{
    while (!m_reader->atEnd()) {
        QXmlStreamReader::TokenType tt = m_reader->readNext();
        qCDebug(scxmlParserLog) << "parse, tt=" << tt;
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
                    addError("unsupporter scxml version, expected 1.0 in scxml tag");
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
                addError("Specification of the initial state through the initial tag not supported.");
                m_stack.append(ParserState(ParserState::Initial));
            } else if (elName == QLatin1String("transition")) {
                if (!checkAttributes(attributes, "|event,cond,target,type")) return;
                m_currentTransition = new ScxmlTransition(m_currentParent,
                                attributes.value("event").toString(),
                                attributes.value("target").toString(),
                                attributes.value("cond").toString());
                ParserState pNew = ParserState(ParserState::Transition);
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
                m_stack.append(ParserState(ParserState::OnEntry));
            } else if (elName == QLatin1String("onexit")) {
                if (!checkAttributes(attributes, "")) return;
                m_stack.append(ParserState(ParserState::OnExit));
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
                ExecutableContent::Raise *raiseI = new ExecutableContent::Raise;
                raiseI->event = attributes.value(QLatin1String("event")).toString();
                pNew.instruction = raiseI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("if")) {
                if (!checkAttributes(attributes, "cond")) return;
                ParserState pNew = ParserState(ParserState::If);
                ExecutableContent::If *ifI = new ExecutableContent::If;
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
                ExecutableContent::Foreach *foreachI = new ExecutableContent::Foreach;
                foreachI->array = attributes.value(QLatin1String("array")).toString();
                foreachI->item = attributes.value(QLatin1String("item")).toString();
                foreachI->index = attributes.value(QLatin1String("index")).toString();
                pNew.instruction = foreachI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("log")) {
                if (!checkAttributes(attributes, "|label,expr")) return;
                ParserState pNew = ParserState(ParserState::Log);
                ExecutableContent::Log *logI = new ExecutableContent::Log;
                logI->label = attributes.value(QLatin1String("label")).toString();
                logI->expr = attributes.value(QLatin1String("expr")).toString();
                pNew.instruction = logI;
                m_stack.append(pNew);
            } else if (elName == QLatin1String("datamodel")) {
                if (!checkAttributes(attributes, "")) return;
                m_stack.append(ParserState(ParserState::DataModel));
            } else if (elName == QLatin1String("data")) {
                if (!checkAttributes(attributes, "id|src,expr")) return;
                QString dataName = attributes.value("id").toString();
                QStringRef src = attributes.value("src");
                QStringRef expr = attributes.value("expr");
                //maybeId()
                m_stack.append(ParserState(ParserState::Data));
            } else if (elName == QLatin1String("assign")) {
                m_stack.append(ParserState(ParserState::Assign));
            } else if (elName == QLatin1String("donedata")) {
                m_stack.append(ParserState(ParserState::DoneData));
            } else if (elName == QLatin1String("content")) {
                m_stack.append(ParserState(ParserState::Content));
            } else if (elName == QLatin1String("param")) {
                m_stack.append(ParserState(ParserState::Param));
            } else if (elName == QLatin1String("script")) {
                m_stack.append(ParserState(ParserState::Script));
            } else if (elName == QLatin1String("send")) {
                m_stack.append(ParserState(ParserState::Send));
            } else if (elName == QLatin1String("cancel")) {
                m_stack.append(ParserState(ParserState::Cancel));
            } else if (elName == QLatin1String("invoke")) {
                m_stack.append(ParserState(ParserState::Invoke));
            } else if (elName == QLatin1String("finalize")) {
                m_stack.append(ParserState(ParserState::Finalize));
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
                m_state = FinishedParsing;
                return;
            case ParserState::State:
                m_currentState = m_currentParent = m_currentParent->parentState();
            case ParserState::Parallel:
                m_currentState = m_currentParent = m_currentParent->parentState();
            case ParserState::Transition:
            case ParserState::Initial:
                m_currentState = m_currentParent;
            case ParserState::Final:
                m_currentState = m_currentParent;
            case ParserState::OnEntry:
            case ParserState::OnExit:
            case ParserState::History:
                m_currentState = m_currentParent;
            case ParserState::Raise:
            case ParserState::If:
            case ParserState::ElseIf:
            case ParserState::Else:
            case ParserState::Foreach:
            case ParserState::Log:
            case ParserState::DataModel:
            case ParserState::Data:
            case ParserState::DataElement:
            case ParserState::Assign:
            case ParserState::DoneData:
            case ParserState::Content:
            case ParserState::Param:
            case ParserState::Script:
            case ParserState::Send:
            case ParserState::Cancel:
            case ParserState::Invoke:
            case ParserState::Finalize:
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
        m_state = ParsingError;
    }
}

void ScxmlParser::addError(const QString &msg, ErrorMessage::Severity severity)
{
    m_errors.append(ErrorMessage(severity, msg, QStringLiteral("%1:%2 %3").arg(m_reader->lineNumber())
                                 .arg(m_reader->columnNumber())
                                 .arg((m_reader->error() != QXmlStreamReader::NoError) ? m_reader->errorString() : QString())));
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
    requiredNames = attrSplit.value(0).split(QLatin1Char(','));
    optionalNames = attrSplit.value(1).split(QLatin1Char(','));
    if (attrSplit.size() > 2) {
        addError("Internal error, invalid attribStr in checkAttributes");
        m_state = ParsingError;
    }
    return checkAttributes(attributes, attribStr);
}

bool ScxmlParser::checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames, QStringList optionalNames)
{
    foreach (const QXmlStreamAttribute &attribute, attributes) {
        QString name = attribute.name().toString();
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
