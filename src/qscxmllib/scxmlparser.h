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

#ifndef SCXMLPARSER_H
#define SCXMLPARSER_H

#include "scxmlstatetable.h"

#include <QStringList>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamAttributes;
class QXmlStreamReader;
class QHistoryState;
class QFile;
QT_END_NAMESPACE

namespace Scxml {

struct ParserState {
    enum Kind {
        Scxml,
        State,
        Parallel,
        Transition,
        Initial,
        Final,
        OnEntry,
        OnExit,
        History,
        Raise,
        If,
        ElseIf,
        Else,
        Foreach,
        Log,
        DataModel,
        Data,
        DataElement,
        Assign,
        DoneData,
        Content,
        Param,
        Script,
        Send,
        Cancel,
        Invoke,
        Finalize,
        None
    };
    Kind kind;
    QString chars;
    ExecutableContent::Instruction *instruction;
    ExecutableContent::InstructionSequence *instructionContainer;
    QByteArray initialId;

    bool collectChars();

    ParserState(Kind kind=None)
        : kind(kind)
        , instruction(0)
        , instructionContainer(0)
    {}
    ~ParserState() { }

    bool validChild(ParserState::Kind child) const;
    static bool validChild(ParserState::Kind parent, ParserState::Kind child);
    static bool isExecutableContent(ParserState::Kind kind);
};

struct ErrorMessage
{
    enum Severity {
        Debug,
        Info,
        Error
    };
    Severity severity;
    QString msg;
    QString parserState;
    ErrorMessage(Severity severity = Severity::Error,
                 const QString &msg = QStringLiteral("UnknownError"),
                 const QString &parserState = QString())
        : severity(severity), msg(msg), parserState(parserState){ }
    QString severityString() const {
        switch (severity) {
        case Debug:
            return QStringLiteral("Debug: ");
        case Info:
            return QStringLiteral("Info: ");
        case Error:
            return QStringLiteral("Error: ");
        }
        return QStringLiteral("Severity%1: ").arg(severity);
    }
};

struct ParsingOptions {
    ParsingOptions() { }
};

class ScxmlParser
{
public:
    typedef std::function<QByteArray(const QString &, bool &, ScxmlParser *parser)> LoaderFunction;
    static LoaderFunction loaderForDir(const QString &basedir);

    enum State {
        StartingParsing,
        ParsingScxml,
        ParsingError,
        FinishedParsing,
    };

    ScxmlParser(QXmlStreamReader *xmlReader, LoaderFunction loader = Q_NULLPTR);
    void parse();
    void addError(const QString &msg, ErrorMessage::Severity severity = ErrorMessage::Error);
    void addError(const char *msg, ErrorMessage::Severity severity = ErrorMessage::Error);
    std::function<bool(const QString &)> errorDumper() {
        return [this](const QString &msg) -> bool { this->addError(msg); return true; };
    }
    StateTable *table() {
        return m_table;
    }

    State state() const { return m_state; }
    QList<ErrorMessage> errors() const { return m_errors; }

private:
    bool maybeId(const QXmlStreamAttributes &attributes, QObject *obj);
    bool checkAttributes(const QXmlStreamAttributes &attributes, const char *attribStr);
    bool checkAttributes(const QXmlStreamAttributes &attributes, QStringList requiredNames,
                         QStringList optionalNames);
    void ensureInitialState(const QByteArray &initialId);

    QState *currentParent() const;

    StateTable *m_table;
    ScxmlTransition *m_currentTransition;
    QAbstractState *m_currentParent;
    QAbstractState *m_currentState;
    LoaderFunction m_loader;
    QStringList m_namespacesToIgnore;

    QXmlStreamReader *m_reader;
    QVector<ParserState> m_stack;
    State m_state;
    QList<ErrorMessage> m_errors;
    ParsingOptions m_options;
};

} // namespace Scxml

#endif // SCXMLPARSER_H
