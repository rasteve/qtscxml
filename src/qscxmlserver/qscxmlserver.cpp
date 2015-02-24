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

#include "../qscxmllib/scxmlparser.h"
#include "../qscxmllib/scxmldumper.h"

#include <QCoreApplication>
#include <QCoreApplication>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QMetaObject>

#include <iostream>

static Q_LOGGING_CATEGORY(scxmlServerLog, "scxml.server")

class Server;

class Session {
public:
    Session(Server *server, const QString &id) : id(id), stateMachine(0), server(server), replySocket(0) { }
    void reachedStableState(bool didChange);
    void runningChanged(bool running);
    void replyFinished(QTcpSocket *socket, QNetworkReply *initialLoad);
    bool handleRequest(QTcpSocket *socket, const QJsonDocument &request);

    QString id;
    Scxml::StateTable *stateMachine;
    Server *server;
    QTcpSocket *replySocket;
};

class Server : public QObject
{
    Q_OBJECT
public:
    static void writeHead(QTcpSocket *socket, int code, const char *key1 =0, const char *value1 = 0,
                   const char *key2 = 0, const char *value2 = 0);
    static void writeHead(QTcpSocket *socket, int code, const QHash<QByteArray, QByteArray> &headers);
    static void writeRest(QTcpSocket *socket, QByteArray r);
    static QByteArray statusPhrase(int code);
    static void errorReply(QTcpSocket *socket, QString msg, int code = 500);
    static void errorReply2(QTcpSocket *socket, QString msg) { errorReply(socket, msg, 500); }

    Server(QObject *parent = 0);
    ~Server();
    bool start(QString port, const QHostAddress &address = QHostAddress::LocalHost);
    void removeSession(const QString &sessionId);
    void handleRequest(QTcpSocket *socket, QByteArray requestLine, QHash<QByteArray, QByteArray> headers, QByteArray data);

    QString newId() {
        return QString::number(++lastSession);
    }
    QTcpServer *server() { return m_server; }

public slots:
    void handleNewConnection();

private:
    QTcpServer *m_server;
    QNetworkAccessManager *manager;
    int lastSession;
    QHash<QString, Session*> sessions;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Server s;
    QStringList args = a.arguments();
    QString port = args.value(1);
    if (port.isEmpty())
        port = QLatin1String("42000");
    if (s.start(port)) {
        qCWarning(scxmlServerLog) << "started scxml server listening on port" << s.server()->serverPort();
    } else {
        qCWarning(scxmlServerLog) << "failed to start scxml server";
    }
    return a.exec();
}

void Session::reachedStableState(bool didChange) {
    Q_UNUSED(didChange);
    if (replySocket) {
        QJsonDocument res;
        QJsonObject obj;
        obj.insert(QStringLiteral("sessionToken"), QJsonValue(id));
        QJsonArray nextConf;
        foreach (const QByteArray &stateId, stateMachine->currentStates())
            nextConf << QJsonValue(QString::fromUtf8(stateId));
        obj.insert(QStringLiteral("nextConfiguration"), nextConf);
        res.setObject(obj);
        QByteArray reply = res.toJson();
        qCDebug(scxmlServerLog) << "reached stable configuration, replying:" << res.toJson();
        Server::writeHead(replySocket, 200, "Content-Type", "application/json");
        Server::writeRest(replySocket, reply);
        replySocket = 0;
    }
}

void Session::runningChanged(bool running) {
    if (!running && replySocket) {
        QJsonDocument res;
        QJsonObject obj;
        obj.insert(QStringLiteral("sessionToken"), QJsonValue(id));
        QJsonArray nextConf;
        foreach (const QByteArray &stateId, stateMachine->currentStates())
            nextConf << QJsonValue(QString::fromUtf8(stateId));
        obj.insert(QStringLiteral("nextConfiguration"), nextConf);
        obj.insert(QStringLiteral("stopped"), QJsonValue(true));
        res.setObject(obj);
        qCDebug(scxmlServerLog) << "machine stopped, replying:" << res.toJson();
        Server::writeHead(replySocket, 200, "Content-Type", "application/json");
        Server::writeRest(replySocket, res.toJson());
        replySocket = 0;
    }
}

void Session::replyFinished(QTcpSocket *socket, QNetworkReply *initialLoad) {
    if (!initialLoad || initialLoad->error() != QNetworkReply::NoError) {
        Server::writeHead(socket, 500, "Content-Type", "text/plain");
        Server::writeRest(socket, QByteArray("Error loading scxml")
                          + (initialLoad ? initialLoad->errorString().toUtf8()
                                         : QByteArray()));
        socket = 0;
        server->removeSession(this->id);
    } else {
        QByteArray scxmlDoc = initialLoad->readAll();
        qCDebug(scxmlServerLog) << "scxml document load finished:" << scxmlDoc;
        QXmlStreamReader xmlReader(scxmlDoc);
        Scxml::ScxmlParser parser(&xmlReader, QString());
        parser.parse();
        if (parser.state() != Scxml::ScxmlParser::FinishedParsing) {
            QByteArray res = QByteArray("Error parsing scxml: \n");
            foreach (const Scxml::ErrorMessage &msg, parser.errors()) {
                res.append(msg.severityString().toUtf8());
                res.append(msg.msg.toUtf8());
                res.append(msg.parserState.toUtf8());
                res.append(QByteArray("\n"));
            }
            if (socket) {
                Server::writeHead(socket, 500, "Content-Type", "text/plain");
                Server::writeRest(socket, res);
                socket = 0;
            } else {
                qCWarning(scxmlServerLog) << "failure without replySocket:" << res;
            }
        } else {
            qCDebug(scxmlServerLog) << "scxml document parsed, trying to run";
            Q_ASSERT(!replySocket);
            replySocket = socket;
            stateMachine = parser.table();
            QObject::connect(stateMachine, &QStateMachine::runningChanged,
                             [this](bool running) { runningChanged(running);});
            QObject::connect(stateMachine, &Scxml::StateTable::reachedStableState,
                             [this](bool didChange) { reachedStableState(didChange);});
            QJSEngine *jsEngine = new QJSEngine;
            stateMachine->init(jsEngine);
            stateMachine->start();
        }
    }
}

bool Session::handleRequest(QTcpSocket *socket, const QJsonDocument &request) {
    QJsonObject event = request.object().value(QLatin1String("event")).toObject();
    QByteArray eventName = event.value(QLatin1String("name")).toString().toUtf8();
    if (!eventName.isEmpty()) {
        Q_ASSERT(!replySocket);
        replySocket = socket;
        Scxml::ScxmlEvent::EventType type = Scxml::ScxmlEvent::External;
        if (event.contains(QLatin1String("type"))) {
            QString typeStr = event.value(QLatin1String("type")).toString();
            if (typeStr.compare(QLatin1String("internal"), Qt::CaseInsensitive) == 0)
                type = Scxml::ScxmlEvent::Internal;
            else if (typeStr.compare(QLatin1String("platform"), Qt::CaseInsensitive) == 0)
                type = Scxml::ScxmlEvent::Platform;
            else if (typeStr.compare(QLatin1String("external"), Qt::CaseInsensitive) != 0)
                qCWarning(scxmlServerLog) << "unexpected event type in " << request.toJson();
        }
        QVariant data;
        // remove ifs and rely on defaults?
        if (event.contains(QLatin1String("data")))
            data = event.value(QLatin1String("data")).toVariant();
        QByteArray sendid;
        if (event.contains(QLatin1String("sendid")))
            sendid = event.value(QLatin1String("sendid")).toString().toUtf8();
        QString origin;
        if (event.contains(QLatin1String("origin")))
            origin = event.value(QLatin1String("origin")).toString();
        QString origintype;
        if (event.contains(QLatin1String("origintype")))
            origintype = event.value(QLatin1String("origintype")).toString();
        QByteArray invokeid;
        if (event.contains(QLatin1String("invokeid")))
            invokeid = event.value(QLatin1String("invokeid")).toString().toUtf8();
        qCDebug(scxmlServerLog) << "submitting event" << eventName;
        stateMachine->submitEvent(eventName, data, type, sendid, origin, origintype, invokeid);
        return true;
    } else {
        Server::writeHead(socket, 500, "Content-Type", "text/plain");
        Server::writeRest(socket, QByteArray("got empty event"));
        return false;
    }
}


Server::Server(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , manager(new QNetworkAccessManager)
    , lastSession(0)
{
    connect(m_server, &QTcpServer::newConnection, this, &Server::handleNewConnection);
}

Server::~Server() {
    QHashIterator<QString, Session *> i(sessions);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    sessions.clear();
}

bool Server::start(QString port, const QHostAddress &address) {
    quint16 sPort = 0;
    if (!port.isEmpty()) {
        bool ok;
        int iPort = port.toInt(&ok);
        if (!ok || iPort < 0 || iPort > 0xFFFF)
            qCWarning(scxmlServerLog) << "invalid port to listen to:" << port;
        else
            sPort = static_cast<quint16>(iPort);
    }
   return  m_server->listen(address, sPort);
}

void Server::removeSession(const QString &sessionId) {
    Session *s = sessions.value(sessionId);
    if (s) {
        delete s;
        sessions.remove(sessionId);
    }
}

void Server::writeHead(QTcpSocket *socket, int code, const char *key1, const char *value1, const char *key2, const char *value2)
{
    QHash<QByteArray, QByteArray> headers;
    if (key1)
        headers.insert(QByteArray(key1), QByteArray(value1));
    if (key2)
        headers.insert(QByteArray(key2), QByteArray(value2));
    writeHead(socket, code, headers);
}

void Server::writeHead(QTcpSocket *socket, int code, const QHash<QByteArray, QByteArray> &headers) {
    socket->write(QStringLiteral("HTTP/1.1 %1 ").arg(code).toUtf8());
    socket->write(statusPhrase(code));
    socket->write("\r\n", 2);
    QHashIterator<QByteArray, QByteArray> i(headers);
    while (i.hasNext()) {
        i.next();
        socket->write(i.key());
        socket->write(QByteArray(":"));
        socket->write(i.value());
        socket->write(QByteArray("\r\n"));
    }
}

void Server::writeRest(QTcpSocket *socket, QByteArray r) {
    socket->write(QByteArray("Transfer-Encoding:identity\r\nContent-Length:"));
    socket->write(QString::number(r.length()).toUtf8());
    socket->write(QByteArray("\r\n\r\n"));
    socket->write(r);
    socket->close();
    socket->deleteLater();
}

QByteArray Server::statusPhrase(int code) {
    static bool didInit = false;
    static QHash<int, QByteArray> _statusPhrases;
    if (!didInit) {
        didInit = true;
        _statusPhrases[100] = QByteArray("Continue");
        _statusPhrases[101] = QByteArray("Switching Protocols");
        _statusPhrases[200] = QByteArray("OK");
        _statusPhrases[201] = QByteArray("Created");
        _statusPhrases[202] = QByteArray("Accepted");
        _statusPhrases[203] = QByteArray("Non-Authoritative Information");
        _statusPhrases[204] = QByteArray("No Content");
        _statusPhrases[205] = QByteArray("Reset Content");
        _statusPhrases[206] = QByteArray("Partial Content");
        _statusPhrases[300] = QByteArray("Multiple Choices");
        _statusPhrases[301] = QByteArray("Moved Permanently");
        _statusPhrases[302] = QByteArray("Found");
        _statusPhrases[303] = QByteArray("See Other");
        _statusPhrases[304] = QByteArray("Not Modified");
        _statusPhrases[305] = QByteArray("Use Proxy");
        _statusPhrases[307] = QByteArray("Temporary Redirect");
        _statusPhrases[400] = QByteArray("Bad Request");
        _statusPhrases[401] = QByteArray("Unauthorized");
        _statusPhrases[402] = QByteArray("Payment Required");
        _statusPhrases[403] = QByteArray("Forbidden");
        _statusPhrases[404] = QByteArray("Not Found");
        _statusPhrases[405] = QByteArray("Method Not Allowed");
        _statusPhrases[406] = QByteArray("Not Acceptable");
        _statusPhrases[407] = QByteArray("Proxy Authentication Required");
        _statusPhrases[408] = QByteArray("Request Time-out");
        _statusPhrases[409] = QByteArray("Conflict");
        _statusPhrases[410] = QByteArray("Gone");
        _statusPhrases[411] = QByteArray("Length Required");
        _statusPhrases[412] = QByteArray("Precondition Failed");
        _statusPhrases[413] = QByteArray("Request Entity Too Large");
        _statusPhrases[414] = QByteArray("Request-URI Too Large");
        _statusPhrases[415] = QByteArray("Unsupported Media Type");
        _statusPhrases[416] = QByteArray("Requested range not satisfiable");
        _statusPhrases[417] = QByteArray("Expectation Failed");
        _statusPhrases[500] = QByteArray("Internal Server Error");
        _statusPhrases[501] = QByteArray("Not Implemented");
        _statusPhrases[502] = QByteArray("Bad Gateway");
        _statusPhrases[503] = QByteArray("Service Unavailable");
        _statusPhrases[504] = QByteArray("Gateway Time-out");
        _statusPhrases[505] = QByteArray("HTTP Version not supported");
    }
    return _statusPhrases.value(code, QByteArray());
}

struct ReadRequest {
    enum State {
        ReadingRequestLine,
        ReadingHeader,
        ReadingData,
        Finished,
        Failed
    };
    typedef std::function<void(QTcpSocket *,QByteArray, QHash<QByteArray,QByteArray>,QByteArray)>
        ContinuationCallback;
    typedef std::function<void(QTcpSocket *, QString)> ErrorCallback;
    typedef void (QAbstractSocket::*ErrorSignal)(QAbstractSocket::SocketError);
    ReadRequest(QTcpSocket *socket, ContinuationCallback continuation, ErrorCallback errorHandler)
        : state(ReadingRequestLine), socket(socket), continuation(continuation)
        , errorHandler(errorHandler) {
        readyReadConnection = QObject::connect(socket, &QTcpSocket::readyRead, [this]() {
            readMore(); });
        errorConnection = QObject::connect(socket,
                                           static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
                                           [this](QAbstractSocket::SocketError error) {
            handleSocketError(error); });
    }

    State state;
    QTcpSocket *socket;
    QByteArray requestLine;
    QHash<QByteArray,QByteArray> headers;
    QByteArray data;
    qint64 dataLength;
    ContinuationCallback continuation;
    ErrorCallback errorHandler;
    QMetaObject::Connection readyReadConnection;
    QMetaObject::Connection errorConnection;
    void handleSocketError(QAbstractSocket::SocketError) {
        QObject::disconnect(readyReadConnection);
        QObject::disconnect(errorConnection);
        QString errorMsg = QLatin1String("error reading request:") + socket->errorString();
        qCWarning(scxmlServerLog) << errorMsg;
        errorHandler(socket, errorMsg);
        destroy();
    }
    void destroy() {
        qCDebug(scxmlServerLog) << "destroying request @" << (size_t)(void *)this;
        delete this;
    }

    void readMore() {
        if (state == Finished || state == Failed) {
            Q_ASSERT(false);
            return;
        }
        bool shouldStop = false;
        while (!shouldStop) {
            char buf[32768];
            qint64 bytesRead = 0;
            qint64 bytesReadNow = -1;
            if (state == ReadingHeader || state == ReadingRequestLine) {
                qint64 toRead = 4;
                int j = data.length();
                if (j > 0) {
                    char c = data.at(--j);
                    if (c == '\n') {
                        if (j > 0 && data.at(--j) == '\r')
                            toRead = 2;
                        else
                            toRead = 4;
                    } else if (c == '\r') {
                        if (j > 1 && data.at(--j) == '\n' && data.at(--j) == '\r')
                            toRead = 1;
                        else
                            toRead = 3;
                    } else {
                        toRead = 4;
                    }
                }
                bool stopReading = false;
                while (bytesRead + 4 < static_cast<qint64>(sizeof(buf)) && !stopReading) {
                    bytesReadNow = socket->read(&buf[bytesRead], toRead);
                    if (bytesReadNow <= 0) {
                        shouldStop = true;
                        stopReading = true;
                        break;
                    }
                    int baseIndex = bytesRead;
                    bytesRead += bytesReadNow;
                    for (int i = 0; i < bytesReadNow; ++i) {
                        char c = buf[baseIndex + i];
                        if (c == '\n') {
                            if (toRead == 1) {
                                // finished headers
                                stopReading = true;
                                break;
                            } else if (toRead == 3) {
                                toRead = 2;
                            } else {
                                qCWarning(scxmlServerLog) << "newlines mismatch in http header '"
                                                          << data << QByteArray(&buf[0], bytesRead) << "'";
                                toRead = 4;
                            }
                        } else if (c == '\r') {
                            if (toRead == 4) {
                                toRead = 3;
                            } else if (toRead == 2) {
                                toRead = 1;
                            } else {
                                qCWarning(scxmlServerLog) << "newlines mismatch in http header '"
                                                          << data << QByteArray(&buf[0], bytesRead) << "'";
                                toRead = 3;
                            }
                        } else {
                            toRead = 4;
                        }
                    }
                }
            } else if (state == ReadingData) {
                qint64 toRead = sizeof(buf);
                if (dataLength >= 0)
                    toRead = std::max(toRead, dataLength - data.length());
                if (toRead < 1) {
                    Q_ASSERT(false);
                    shouldStop = true;
                } else {
                    bytesReadNow = socket->read(&buf[0], toRead);
                }
                if (bytesReadNow > 0)
                    bytesRead = bytesReadNow;
            }
            if (bytesRead > 0) {
                data.append(&buf[0], bytesRead);
                if (!process())
                    shouldStop = true;
            }
            if (bytesReadNow == -1) {
                if (state == ReadingData && dataLength == -1) {
                    state = Finished;
                    continuation(socket, requestLine, headers, data);
                } else if (state != Finished) {
                    state = Failed;
                    errorHandler(socket, QLatin1String("Failed to parse request"));
                }
            } else if (bytesRead == 0) {
                shouldStop = true;
            }
        }
        if (state  == Finished || state == Failed) {
            QObject::disconnect(readyReadConnection);
            QObject::disconnect(errorConnection);
            destroy();
        }
    }

    bool process() {
        switch (state) {
        case ReadingRequestLine:
            processRequestLine();
            if (state != ReadingHeader)
                break;
        case ReadingHeader:
            processHeaders();
            if (state != ReadingData)
                break;
        case ReadingData:
            if (data.length() >= dataLength) {
                if (data.length() > dataLength) {
                    qCWarning(scxmlServerLog) << "read too much";
                    data.resize(dataLength);
                }
                state = Finished;
                continuation(socket, requestLine, headers, data);
                return false;
            }
            break;
        case Finished:
        case Failed:
            return false;
        }
        return true;
    }

    void processRequestLine() {
        Q_ASSERT(state == ReadingRequestLine);
        int lineEnd = data.indexOf("\r\n");
        if (lineEnd != -1) {
            requestLine = data.left(lineEnd);
            data = data.mid(lineEnd);
            state = ReadingHeader;
        }
    }

    void processHeaders() {
        Q_ASSERT(state == ReadingHeader);
        int endHeader = data.indexOf("\r\n\r\n");
        if (endHeader >= 0) {
            QByteArray headerData = data.right(endHeader);
            data = data.mid(endHeader + 4);
            state = ReadingData;
            int fromI = 0;
            while (fromI < headerData.length()) {
                int toI = headerData.indexOf("\r\n", fromI);
                if (toI < 0) toI = headerData.length();
                int oldFromI = fromI;
                fromI = toI + 2;
                if (oldFromI == toI) // empty header, (can happen for the first header)
                    continue;
                char c = headerData.at(oldFromI);
                if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
                    qCWarning(scxmlServerLog) << "skipping invalid header with spaces at the beginning, char "
                                              << oldFromI << " of '" << headerData << "'";
                    continue;
                }
                int headerSplitI = headerData.indexOf(':', oldFromI);
                if (headerSplitI < oldFromI || headerSplitI > toI) {
                    qCWarning(scxmlServerLog) << "skipping invalid header missing ':' at char "
                                              << oldFromI << " of '" << headerData << "'";
                    continue;
                }
                QByteArray key = headerData.mid(oldFromI, headerSplitI - oldFromI).trimmed();
                for (int i = 0; i < key.length(); ++i) { // lowercase
                    char c = key.at(i);
                    if (c >= 'A' && c <= 'Z')
                        key[i] = c - 'A' + 'a';
                }
                while (++headerSplitI < toI && (headerData.at(headerSplitI) == ' '
                                                || headerData.at(headerSplitI) == '\t'));
                while (fromI < headerData.length()
                       && (headerData.at(fromI) == ' ' || headerData.at(fromI) == '\t')) {
                    // collect multi line values
                    toI = headerData.indexOf("\r\n", fromI);
                    if (toI < 0) toI = headerData.length();
                    fromI = toI + 2;
                }
                QByteArray value = headerData.mid(headerSplitI, toI - headerSplitI);
                if (headers.contains(key)) {
                    headers[key].append(QByteArray(","));
                    headers[key].append(value);
                } else {
                    headers.insert(key, value);
                }
            }
            QByteArray cLength = headers.value(QByteArray("content-length"));
            QByteArray cEncoding = headers.value(QByteArray("transfer-encoding"));
            if (!cEncoding.isEmpty()) {
                if (cEncoding != QByteArray("identity")) {
                    dataLength = -1;
                } else {
                    state = Failed;
                    errorHandler(socket,
                                 QStringLiteral("Only identiy Transfer encoding supported in request, not '%1'")
                                 .arg(QString::fromUtf8(cEncoding)));
                    return;
                }
            }
            if (!cLength.isEmpty()) {
                bool ok;
                dataLength = QString::fromUtf8(cLength).trimmed().toLong(&ok);
                if (!ok) {
                    state = Failed;
                    errorHandler(socket,
                                 QStringLiteral("Failed to parse Content-Length from '%1'")
                                 .arg(QString::fromUtf8(cLength)));
                    return;
                }
            }
        }
    }
};

void Server::errorReply(QTcpSocket *socket, QString msg, int code)
{
    qCWarning(scxmlServerLog) << "Error reply:" << msg;
    if (socket) {
        writeHead(socket, code, "Content-Type", "text/plain");
        writeRest(socket, msg.toUtf8());
        socket->deleteLater();
    }
}

void Server::handleNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *newConnection = m_server->nextPendingConnection();
        new ReadRequest(newConnection, [this](QTcpSocket *socket, QByteArray requestLine, QHash<QByteArray, QByteArray> headers, QByteArray data) {
            handleRequest(socket, requestLine, headers, data); },
        &Server::errorReply2);
    }
}

void Server::handleRequest(QTcpSocket *socket, QByteArray requestLine, QHash<QByteArray, QByteArray> headers, QByteArray data)
{
    Q_UNUSED(requestLine);
    Q_UNUSED(headers);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(scxmlServerLog) << "parsing error evaluating request:" << error.errorString();
        writeHead(socket, 500, "Content-Type", "text/plain");
        writeRest(socket, error.errorString().toUtf8());
    } else if (!doc.isObject()) {
        qCWarning(scxmlServerLog) << "request is not a json object.";
        writeHead(socket, 500, "Content-Type", "text/plain");
        writeRest(socket, QByteArray("request is not a json object."));
    } else if (doc.object().contains(QLatin1String("load"))) {
        QString loadUrl = doc.object().value(QLatin1String("load")).toString();
        qCDebug(scxmlServerLog) << "requesting scxml from " << loadUrl;
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(loadUrl)));
        Session *newSession = new Session(this, newId());
        sessions.insert(newSession->id, newSession);
        QMetaObject::Connection c = QObject::connect(reply, &QNetworkReply::finished, [reply, newSession, socket](){
            newSession->replyFinished(socket, reply);
        });
        if (reply->isFinished()) {
            QObject::disconnect(c);
            newSession->replyFinished(socket, reply);
        }
    } else if (doc.object().contains(QLatin1String("sessionToken"))) {
        Session *session = sessions.value(doc.object().value(QLatin1String("sessionToken")).toString());
        if (session) {
            qCDebug(scxmlServerLog) << "forwarding request:" << doc.toJson();
            session->handleRequest(socket, doc);
        } else {
            QByteArray errorMsg = QStringLiteral("Could not find session '%1'")
                    .arg(doc.object().value(QLatin1String("sessionToken")).toString())
                    .toUtf8();
            qCWarning(scxmlServerLog) << errorMsg;
            writeHead(socket, 500, "Content-Type", "text/plain");
            writeRest(socket, errorMsg);
        }
    } else {
        qCWarning(scxmlServerLog) << "unknown request:" << error.errorString();
        writeHead(socket, 400, "Content-Type", "text/plain");
        writeRest(socket, QByteArray("Unrecognized request.\n"));
    }
}

#include "qscxmlserver.moc"
