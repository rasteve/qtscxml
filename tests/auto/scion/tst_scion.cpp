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
#include <QJsonDocument>

#include <QtScxml/scxmlparser.h>
#include <QtScxml/ecmascriptdatamodel.h>
#include <QtScxml/nulldatamodel.h>

#include "scxml/scion.h"
#include "scxml/compiled_tests.h"

Q_DECLARE_METATYPE(std::function<Scxml::StateMachine *()>);

enum { SpyWaitTime = 8000 };

static QSet<QString> weFailOnThese = QSet<QString>()
        // The following test needs manual inspection of the result. However, note that we do not support multiple identical keys for event data.
        << QLatin1String("w3c-ecma/test178.txml")
        // Currently we do not support loading data as XML content inside the <data> tag.
        << QLatin1String("w3c-ecma/test557.txml")
        // FIXME: Currently we do not support loading data from a src.
        << QLatin1String("w3c-ecma/test552.txml")
        << QLatin1String("w3c-ecma/test558.txml")
        // A nested state machine is used, which we do not support.
        << QLatin1String("w3c-ecma/test187.txml")
        // We do not support the optional basic http event i/o processor.
        << QLatin1String("w3c-ecma/test201.txml")
        << QLatin1String("w3c-ecma/test207.txml")
        // The following test uses the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441a.txml")
        // The following test needs manual inspection of the result. However, note that we do not support the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441b.txml")
        // The following test needs manual inspection of the result.
        << QLatin1String("w3c-ecma/test307.txml")
        // We do not (yet?) support invoke.
        << QLatin1String("w3c-ecma/test215.txml")
        << QLatin1String("w3c-ecma/test216.txml")
        << QLatin1String("w3c-ecma/test223.txml")
        << QLatin1String("w3c-ecma/test224.txml")
        << QLatin1String("w3c-ecma/test225.txml")
        << QLatin1String("w3c-ecma/test226.txml")
        << QLatin1String("w3c-ecma/test228.txml")
        << QLatin1String("w3c-ecma/test229.txml")
        << QLatin1String("w3c-ecma/test230.txml")
        << QLatin1String("w3c-ecma/test232.txml")
        << QLatin1String("w3c-ecma/test233.txml")
        << QLatin1String("w3c-ecma/test234.txml")
        << QLatin1String("w3c-ecma/test235.txml")
        << QLatin1String("w3c-ecma/test236.txml")
        << QLatin1String("w3c-ecma/test237.txml")
        << QLatin1String("w3c-ecma/test238.txml")
        << QLatin1String("w3c-ecma/test239.txml")
        << QLatin1String("w3c-ecma/test240.txml")
        << QLatin1String("w3c-ecma/test241.txml")
        << QLatin1String("w3c-ecma/test243.txml")
        << QLatin1String("w3c-ecma/test244.txml")
        << QLatin1String("w3c-ecma/test245.txml")
        << QLatin1String("w3c-ecma/test247.txml")
        << QLatin1String("w3c-ecma/test242.txml")
        << QLatin1String("w3c-ecma/test276.txml")
        << QLatin1String("w3c-ecma/test253.txml")
        << QLatin1String("w3c-ecma/test252.txml")
        << QLatin1String("w3c-ecma/test250.txml")
        << QLatin1String("w3c-ecma/test338.txml")
        << QLatin1String("w3c-ecma/test422.txml")
        << QLatin1String("w3c-ecma/test530.txml")
        << QLatin1String("w3c-ecma/test554.txml")
           ;

static QSet<QString> weDieOnThese = QSet<QString>()
        << QLatin1String("send-data/send1") // test suite problem: we expect every stable configuration to be listed.
        << QLatin1String("delayedSend/send1") // same as above
        << QLatin1String("delayedSend/send2") // same as above
        << QLatin1String("delayedSend/send3") // same as above
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0) // QTBUG-46703
        << QLatin1String("history/history3")
        << QLatin1String("history/history4")
        << QLatin1String("history/history5")
#endif // Qt 5.6.0
        << QLatin1String("w3c-ecma/test364.txml") // initial attribute on <state>
        << QLatin1String("w3c-ecma/test388.txml") // Qt refuses to set an initial state to a "deep" state
        << QLatin1String("w3c-ecma/test403a.txml")
        << QLatin1String("w3c-ecma/test403c.txml")
           ;

static QSet<QString> differentSemantics = QSet<QString>()
        // FIXME: looks like a bug in internal event ordering when writing to read-only variables.
        << QLatin1String("w3c-ecma/test329.txml")
        << QLatin1String("w3c-ecma/test346.txml")
        // Scion apparently sets <data> values without a src/expr attribute to 0. We set it to undefined, as specified in B.2.1.
        << QLatin1String("w3c-ecma/test456.txml") // replaced by modified_test456
        // Qt does not support forcing initial states that are not marked as such.
        << QLatin1String("w3c-ecma/test413.txml") // FIXME: verify initial state setting...
        << QLatin1String("w3c-ecma/test576.txml")
        // FIXME: Currently we do not support loading scripts from a src.
        << QLatin1String("w3c-ecma/test301.txml")
        << QLatin1String("script-src/test0")
        << QLatin1String("script-src/test1")
        << QLatin1String("script-src/test2")
        << QLatin1String("script-src/test3")
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

using namespace Scxml;

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
    bool runTest(StateMachine *stateMachine, const QJsonObject &testDescription);
};

void TestScion::initTestCase()
{
}

enum TestStatus {
    TestIsOk,
    TestFails,
    TestCrashes,
    TestUsesDifferentSemantics
};
Q_DECLARE_METATYPE(TestStatus)

void TestScion::generateData()
{
    QTest::addColumn<QString>("scxml");
    QTest::addColumn<QString>("json");
    QTest::addColumn<TestStatus>("testStatus");
    QTest::addColumn<std::function<Scxml::StateMachine *()>>("creator");

    const int nrOfTests = sizeof(testBases) / sizeof(const char *);
    for (int i = 0; i < nrOfTests; ++i) {
        TestStatus testStatus;
        QString base = QString::fromUtf8(testBases[i]);
        if (weDieOnThese.contains(base))
            testStatus = TestCrashes;
        else if (differentSemantics.contains(base))
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
    QFETCH(std::function<Scxml::StateMachine *()>, creator);

//    fprintf(stderr, "\n\n%s\n%s\n\n", qPrintable(scxml), qPrintable(json));

    if (testStatus == TestCrashes)
        QSKIP("Test is marked as a crasher");
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
    ScxmlParser parser(&xmlReader);
    parser.parse();
    if (testStatus == TestFails && parser.state() != ScxmlParser::FinishedParsing)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QCOMPARE(parser.state(), ScxmlParser::FinishedParsing);
    QVERIFY(parser.errors().isEmpty());
    scxmlFile.close();

    QScopedPointer<StateMachine> table(parser.instantiateStateMachine());
    if (table == Q_NULLPTR && testStatus == TestFails) {
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    }
    QVERIFY(table != Q_NULLPTR);
    parser.instantiateDataModel(table.data());

    if (testStatus == TestFails)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QVERIFY(runTest(table.data(), testDescription.object()));
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
    QFETCH(std::function<Scxml::StateMachine *()>, creator);

    if (testStatus == TestCrashes)
        QSKIP("Test is marked as a crasher");
    if (testStatus == TestUsesDifferentSemantics)
        QSKIP("Test uses different semantics");

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

    QScopedPointer<Scxml::StateMachine> table(creator());
    if (table == Q_NULLPTR && testStatus == TestFails) {
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    }
    QVERIFY(table != Q_NULLPTR);

    if (testStatus == TestFails)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QVERIFY(runTest(table.data(), testDescription.object()));
}

static bool verifyStates(StateMachine *stateMachine, const QJsonObject &stateDescription, const QString &key, int counter)
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

static bool playEvent(StateMachine *stateMachine, const QJsonObject &eventDescription, int counter)
{
    if (!stateMachine->isRunning()) {
        qWarning() << "State machine stopped running!";
        return false;
    }

    Q_ASSERT(eventDescription.contains(QLatin1String("event")));
    auto event = eventDescription.value(QLatin1String("event")).toObject();
    auto eventName = event.value(QLatin1String("name")).toString().toUtf8();
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
    QVariantList dataValues;
    QStringList dataNames;
    // remove ifs and rely on defaults?
    if (event.contains(QLatin1String("data"))) {
        QJsonValue dataVal = event.value(QLatin1String("data"));
        if (dataVal.isObject()) {
            QJsonObject dataObj = dataVal.toObject();
            for (QJsonObject::const_iterator i = dataObj.constBegin(); i != dataObj.constEnd(); ++i) {
                dataNames.append(i.key());
                dataValues.append(i.value().toVariant());
            }
        } else {
            dataValues.append(dataVal.toVariant());
        }
    }
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
    QScxmlEvent *e = new QScxmlEvent;
    e->setName(eventName);
    e->setEventType(type);
    e->setDataValues(dataValues);
    e->setDataNames(dataNames);
    e->setSendId(sendid);
    e->setOrigin(origin);
    e->setOriginType(origintype);
    e->setInvokeId(invokeid);
//    qDebug() << "submitting event" << eventName << "....";
    if (eventDescription.contains(QLatin1String("after"))) {
        int delay = eventDescription.value(QLatin1String("after")).toInt();
        Q_ASSERT(delay > 0);
        stateMachine->submitDelayedEvent(delay, e);
    } else {
        stateMachine->submitEvent(e);
    }

    if (!MySignalSpy(stateMachine, SIGNAL(reachedStableState(bool))).fastWait()) {
        qWarning() << "State machine did not reach a stable state!";
    } else if (verifyStates(stateMachine, eventDescription, QLatin1String("nextConfiguration"), counter)) {
        return true;
    }

    qWarning() << "... after sending event" << event;
    return false;
}

static bool playEvents(StateMachine *stateMachine, const QJsonObject &testDescription)
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

bool TestScion::runTest(StateMachine *stateMachine, const QJsonObject &testDescription)
{
    MySignalSpy stableStateSpy(stateMachine, SIGNAL(reachedStableState(bool)));
    MySignalSpy finishedSpy(stateMachine, SIGNAL(finished()));

    if (!stateMachine->init()) {
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
