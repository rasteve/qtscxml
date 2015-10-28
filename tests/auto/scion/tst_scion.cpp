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

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QJsonDocument>

#include <QtScxml/qscxmlparser.h>
#include <QtScxml/qscxmlecmascriptdatamodel.h>
#include <QtScxml/qscxmlnulldatamodel.h>

#include <functional>

#include "scxml/scion.h"
#include "scxml/compiled_tests.h"

Q_DECLARE_METATYPE(std::function<QScxmlStateMachine *()>);

enum { SpyWaitTime = 12000 };

static QSet<QString> weFailOnThese = QSet<QString>()
        // The following test needs manual inspection of the result. However, note that we do not support multiple identical keys for event data.
        << QLatin1String("delayedSend/send1") // same as above
        << QLatin1String("delayedSend/send2") // same as above
        << QLatin1String("delayedSend/send3") // same as above
        << QLatin1String("send-data/send1") // test suite problem: we expect every stable configuration to be listed.
        << QLatin1String("w3c-ecma/test178.txml")
        // We do not support the optional basic http event i/o processor.
        << QLatin1String("w3c-ecma/test201.txml")
        << QLatin1String("w3c-ecma/test364.txml") // initial attribute on <state>
        << QLatin1String("w3c-ecma/test388.txml") // Qt refuses to set an initial state to a "deep" state

        // Currently we do not support loading data as XML content inside the <data> tag.
        << QLatin1String("w3c-ecma/test557.txml")
        // The following test uses the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441a.txml")
        // The following test needs manual inspection of the result. However, note that we do not support the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441b.txml")
        // The following test needs manual inspection of the result.
        << QLatin1String("w3c-ecma/test230.txml")
        << QLatin1String("w3c-ecma/test250.txml")
        << QLatin1String("w3c-ecma/test307.txml")
           ;

static QSet<QString> differentSemantics = QSet<QString>()
        // FIXME: looks like a bug in internal event ordering when writing to read-only variables.
        << QLatin1String("w3c-ecma/test329.txml")
        // Qt does not support forcing initial states that are not marked as such.
        << QLatin1String("w3c-ecma/test413.txml") // FIXME: verify initial state setting...
        << QLatin1String("w3c-ecma/test576.txml") // FIXME: verify initial state setting...
        // Scion apparently sets <data> values without a src/expr attribute to 0. We set it to undefined, as specified in B.2.1.
        << QLatin1String("w3c-ecma/test456.txml") // replaced by modified_test456
        // FIXME: qscxmlc fails on improper scxml file, currently no way of testing it properly for compiled case
        << QLatin1String("w3c-ecma/test301.txml")
        // FIXME: Currently we do not support loading scripts from a srcexpr.
        << QLatin1String("w3c-ecma/test216.txml")
        // FIXME: Currently we do not support nested scxml as a child of assign.
        << QLatin1String("w3c-ecma/test530.txml")
        ;

class MySignalSpy: public QSignalSpy
{
public:
    explicit MySignalSpy(const QObject *obj, const char *aSignal)
        : QSignalSpy(obj, aSignal)
    {}

    bool fastWait()
    {
        const int increment = SpyWaitTime / 20;
        for (int total = 0; total < SpyWaitTime; total += increment) {
            if (this->wait(increment))
                return true;
        }
        return false;
    }
};

class DynamicLoader: public QScxmlParser::Loader
{
public:
    DynamicLoader(QScxmlParser *parser);
    QByteArray load(const QString &name, const QString &baseDir, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;

};

DynamicLoader::DynamicLoader(QScxmlParser *parser)
    : Loader(parser)
{}

QByteArray DynamicLoader::load(const QString &name, const QString &baseDir, bool *ok)
{
    Q_ASSERT(ok != nullptr);

    *ok = false;
    QUrl url(name);
    if (!url.isLocalFile() && !url.isRelative())
        parser()->addError(QStringLiteral("src attribute is not a local file (%1)").arg(name));
    QFileInfo fInfo = url.isLocalFile() ? url.toLocalFile() : name;
    if (fInfo.isRelative())
        fInfo = QFileInfo(QDir(baseDir).filePath(fInfo.filePath()));
    fInfo = QFileInfo(QLatin1String(":/") + fInfo.filePath()); // take it from resources

    if (!fInfo.exists()) {
        parser()->addError(QStringLiteral("src attribute resolves to non existing file (%1)").arg(fInfo.absoluteFilePath()));
    } else {
        QFile f(fInfo.absoluteFilePath());
        if (f.open(QFile::ReadOnly)) {
            *ok = true;
            QByteArray array = f.readAll();
            return array;
        } else {
            parser()->addError(QStringLiteral("Failure opening file %1: %2")
                               .arg(fInfo.absoluteFilePath(), f.errorString()));
        }
    }
    return QByteArray();
}


class TestScion: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void dynamic_data();
    void dynamic();
    void compiled_data();
    void compiled();

private:
    void generateData();
    bool runTest(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription);
};

void TestScion::initTestCase()
{
}

enum TestStatus {
    TestIsOk,
    TestFails,
    TestUsesDifferentSemantics
};
Q_DECLARE_METATYPE(TestStatus)

void TestScion::generateData()
{
    QTest::addColumn<QString>("scxml");
    QTest::addColumn<QString>("json");
    QTest::addColumn<TestStatus>("testStatus");
    QTest::addColumn<std::function<QScxmlStateMachine *()>>("creator");

    const int nrOfTests = sizeof(testBases) / sizeof(const char *);
    for (int i = 0; i < nrOfTests; ++i) {
        TestStatus testStatus;
        QString base = QString::fromUtf8(testBases[i]);
        if (differentSemantics.contains(base))
            testStatus = TestUsesDifferentSemantics;
        else if (weFailOnThese.contains(base))
            testStatus = TestFails;
        else
            testStatus = TestIsOk;
        QTest::newRow(testBases[i]) << base + QLatin1String(".scxml")
                                    << base + QLatin1String(".json")
                                    << testStatus
                                    << creators[i];
    }
}

void TestScion::dynamic_data()
{
    generateData();
}

void TestScion::dynamic()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(TestStatus, testStatus);
    QFETCH(std::function<QScxmlStateMachine *()>, creator);

//    fprintf(stderr, "\n\n%s\n%s\n\n", qPrintable(scxml), qPrintable(json));

    if (testStatus == TestUsesDifferentSemantics)
        QSKIP("Test uses different semantics");

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
//    fprintf(stderr, "test description: %s\n", testDescription.toJson().constData());
    jsonFile.close();
    QVERIFY(testDescription.isObject());

    QFile scxmlFile(QLatin1String(":/") + scxml);
    QVERIFY(scxmlFile.open(QIODevice::ReadOnly));
    QXmlStreamReader xmlReader(&scxmlFile);
    QScxmlParser parser(&xmlReader);
    parser.setFileName(scxml);
    DynamicLoader loader(&parser);
    parser.setLoader(&loader);
    parser.parse();
    if (testStatus == TestFails && parser.state() != QScxmlParser::FinishedParsing)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QCOMPARE(parser.state(), QScxmlParser::FinishedParsing);
    QVERIFY(parser.errors().isEmpty());
    scxmlFile.close();

    QScopedPointer<QScxmlStateMachine> stateMachine(parser.instantiateStateMachine());
    if (stateMachine == Q_NULLPTR && testStatus == TestFails) {
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    }
    QVERIFY(stateMachine != Q_NULLPTR);
    parser.instantiateDataModel(stateMachine.data());

    if (testStatus == TestFails)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QVERIFY(runTest(stateMachine.data(), testDescription.object()));
    QCoreApplication::processEvents(); // flush any pending events
}

static QStringList getStates(const QJsonObject &obj, const QString &key)
{
    QStringList states;
    auto jsonStates = obj.value(key).toArray();
    for (int i = 0, ei = jsonStates.size(); i != ei; ++i) {
        QString state = jsonStates.at(i).toString();
        Q_ASSERT(!state.isEmpty());
        states.append(state);
    }
    std::sort(states.begin(), states.end());
    return states;
}

void TestScion::compiled_data()
{
    generateData();
}

void TestScion::compiled()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(TestStatus, testStatus);
    QFETCH(std::function<QScxmlStateMachine *()>, creator);

    if (testStatus == TestUsesDifferentSemantics)
        QSKIP("Test uses different semantics");

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

    QScopedPointer<QScxmlStateMachine> stateMachine(creator());
    if (stateMachine == Q_NULLPTR && testStatus == TestFails) {
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    }
    QVERIFY(stateMachine != Q_NULLPTR);

    if (testStatus == TestFails)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QVERIFY(runTest(stateMachine.data(), testDescription.object()));
    QCoreApplication::processEvents(); // flush any pending events
}

static bool verifyStates(QScxmlStateMachine *stateMachine, const QJsonObject &stateDescription, const QString &key, int counter)
{
    auto current = stateMachine->activeStates();
    std::sort(current.begin(), current.end());
    auto expected = getStates(stateDescription, key);
    if (current == expected)
        return true;

    qWarning("Incorrect %s (%d)!", qPrintable(key), counter);
    qWarning() << "Current configuration:" << current;
    qWarning() << "Expected configuration:" << expected;
    return false;
}

static bool playEvent(QScxmlStateMachine *stateMachine, const QJsonObject &eventDescription, int counter)
{
    if (!stateMachine->isRunning()) {
        qWarning() << "State machine stopped running!";
        return false;
    }

    Q_ASSERT(eventDescription.contains(QLatin1String("event")));
    auto event = eventDescription.value(QLatin1String("event")).toObject();
    auto eventName = event.value(QLatin1String("name")).toString();
    Q_ASSERT(!eventName.isEmpty());
    QScxmlEvent::EventType type = QScxmlEvent::ExternalEvent;
    if (event.contains(QLatin1String("type"))) {
        QString typeStr = event.value(QLatin1String("type")).toString();
        if (typeStr.compare(QLatin1String("external"), Qt::CaseInsensitive) == 0)
            type = QScxmlEvent::InternalEvent;
        else if (typeStr.compare(QLatin1String("platform"), Qt::CaseInsensitive) == 0)
            type = QScxmlEvent::PlatformEvent;
        else {
            qWarning() << "unexpected event type in " << eventDescription;
            return false;
        }
    }
    QVariant data;
    // remove ifs and rely on defaults?
    if (event.contains(QLatin1String("data"))) {
        data = event.value(QLatin1String("data")).toVariant();
    }
    QString sendid;
    if (event.contains(QLatin1String("sendid")))
        sendid = event.value(QLatin1String("sendid")).toString();
    QString origin;
    if (event.contains(QLatin1String("origin")))
        origin = event.value(QLatin1String("origin")).toString();
    QString origintype;
    if (event.contains(QLatin1String("origintype")))
        origintype = event.value(QLatin1String("origintype")).toString();
    QString invokeid;
    if (event.contains(QLatin1String("invokeid")))
        invokeid = event.value(QLatin1String("invokeid")).toString();
    QScxmlEvent *e = new QScxmlEvent;
    e->setName(eventName);
    e->setEventType(type);
    e->setData(data);
    e->setSendId(sendid);
    e->setOrigin(origin);
    e->setOriginType(origintype);
    e->setInvokeId(invokeid);
    if (eventDescription.contains(QLatin1String("after"))) {
        int delay = eventDescription.value(QLatin1String("after")).toInt();
        Q_ASSERT(delay > 0);
        e->setDelay(delay);
    }
    stateMachine->submitEvent(e);

    if (!MySignalSpy(stateMachine, SIGNAL(reachedStableState(bool))).fastWait()) {
        qWarning() << "State machine did not reach a stable state!";
    } else if (verifyStates(stateMachine, eventDescription, QLatin1String("nextConfiguration"), counter)) {
        return true;
    }

    qWarning() << "... after sending event" << event;
    return false;
}

static bool playEvents(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription)
{
    auto jsonEvents = testDescription.value(QLatin1String("events"));
    Q_ASSERT(!jsonEvents.isNull());
    auto eventsArray = jsonEvents.toArray();
    for (int i = 0, ei = eventsArray.size(); i != ei; ++i) {
        if (!playEvent(stateMachine, eventsArray.at(i).toObject(), i + 1))
            return false;
    }
    return true;
}

bool TestScion::runTest(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription)
{
    MySignalSpy stableStateSpy(stateMachine, SIGNAL(reachedStableState(bool)));
    MySignalSpy finishedSpy(stateMachine, SIGNAL(finished()));

    if (!stateMachine->init() && stateMachine->name() != QStringLiteral("test487")) {
        // test487 relies on a failing init to see if an error event gets posted.
        qWarning() << "init failed";
        return false;
    }
    stateMachine->start();

    if (testDescription.contains(QLatin1String("events"))
            && !testDescription.value(QLatin1String("events")).toArray().isEmpty()) {
        if (!stableStateSpy.fastWait()) {
            qWarning() << "Failed to reach stable initial state!";
            return false;
        }

        if (!verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration"), 0))
            return false;

        return playEvents(stateMachine, testDescription);
    } else {
        // Wait for all events (delayed or otherwise) to propagate.
        finishedSpy.fastWait(); // Some tests don't have a final state, so don't check for the result.
        return verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration"), 0);
    }
}

QTEST_GUILESS_MAIN(TestScion)
#include "tst_scion.moc"
