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

#include <QScxmlLib/scxmlparser.h>

#include "../3rdparty/scion.h"

enum { SpyWaitTime = 5000 };

static QSet<QString> weFailOnThese = QSet<QString>()
        // The following test needs manual inspection of the result. However, note that we do not support multiple identical keys for event data.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test178.txml")
        // Currently we do not support loading data as XML content inside the <data> tag.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test557.txml")
        // FIXME: Currently we do not support loading data from a src.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test552.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test558.txml")
        // FIXME: Currently we do not support loading scripts from a src.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test301.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/script-src/test0")
        << QLatin1String("scion-tests/scxml-test-framework/test/script-src/test1")
        << QLatin1String("scion-tests/scxml-test-framework/test/script-src/test2")
        << QLatin1String("scion-tests/scxml-test-framework/test/script-src/test3")
        // A nested state machine is used, which we do not support.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test187.txml")
        // We do not support the optional basic http event i/o processor.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test201.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        // The following test uses the undocumented "exmode" attribute.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test441a.txml")
        // The following test needs manual inspection of the result. However, note that we do not support the undocumented "exmode" attribute.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test441b.txml")
        // The following test needs manual inspection of the result.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test307.txml")
        // FIXME: We do not yet support declaring variables in a <script> on any level.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test304.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test303.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test302.txml")
        // We do not (yet?) support invoke.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test215.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test216.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test220.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test223.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test224.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test225.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test226.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test228.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test229.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test230.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test232.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test233.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test234.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test235.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test236.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test237.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test238.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test239.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test243.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test244.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test245.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test247.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test242.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test276.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test252.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test250.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test338.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test422.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test530.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test554.txml")
        // TODO: I don't understand what the next test is supposed to check.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test159.txml")
        // FIXME: the way we dequeue events that are generated before the state machine is started, is wrong: it's too late, because the machine will already have taken initial transitions.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test487.txml")
        // FIXME: UNKNOWN PROBLEM
        << QLatin1String("scion-tests/scxml-test-framework/test/parallel+interrupt/test7")
        << QLatin1String("scion-tests/scxml-test-framework/test/parallel+interrupt/test21")
        << QLatin1String("scion-tests/scxml-test-framework/test/parallel+interrupt/test26")
        << QLatin1String("scion-tests/scxml-test-framework/test/more-parallel/test2")
        << QLatin1String("scion-tests/scxml-test-framework/test/more-parallel/test3")
        << QLatin1String("scion-tests/scxml-test-framework/test/more-parallel/test6")
           ;

static QSet<QString> weDieOnThese = QSet<QString>()
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send1")
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send2")
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send3")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history3") // infinite loop?
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history5") // infinite loop?
        << QLatin1String("scion-tests/scxml-test-framework/test/send-data/send1") // test suite problem: we expect every stable configuration to be listed.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test342.txml") // bug in eventexpr?
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test364.txml") // initial attribute on <state>
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test387.txml") // crash due to assert in the parser
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test388.txml") // same as 387
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test208.txml") // <cancel> seems to have a problem
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test210.txml") // sendidexpr attribute in <cancel>
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test403a.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test403c.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test521.txml") // undispatchable event
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test562.txml") // space normalization
        // TODO: we don't implement <foreach> yet.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test150.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test151.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test152.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test153.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test155.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test156.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test525.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/foreach/test1")
           ;

static QSet<QString> differentSemantics = QSet<QString>()
        // Qt does not support initial transitions in a history state:
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history0")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history1")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history2")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history4")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history6")
        // The data model does not mark the system variables as read-only:
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test322.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test324.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test326.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test329.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test346.txml")
        // Qt does not support internal transitions:
        << QLatin1String("scion-tests/scxml-test-framework/test/internal-transitions/test0")
        << QLatin1String("scion-tests/scxml-test-framework/test/internal-transitions/test1")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test533.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test506.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test505.txml")
        // internal event by <raise>
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test330.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test421.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test423.txml")
        // internal event by raising an error
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test401.txml")
        // Scion apparently sets <data> values without a src/expr attribute to 0. We set it to undefined, as specified in B.2.1.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test456.txml") // replaced by modified_test456
        // Qt does not support forcing initial states that are not marked as such.
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test413.txml") // FIXME: verify initial state setting...
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test576.txml")
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
    void scion_data();
    void scion();

private:
    bool runTest(StateTable *stateMachine, const QJsonObject &testDescription);
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

void TestScion::scion_data()
{
    QTest::addColumn<QString>("scxml");
    QTest::addColumn<QString>("json");
    QTest::addColumn<TestStatus>("testStatus");

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
                                    << testStatus;
    }
}

void TestScion::scion()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(TestStatus, testStatus);

    fprintf(stderr, "\n\n%s\n\n", qPrintable(json));

    if (testStatus == TestCrashes)
        QSKIP("Test is marked as a crasher");
    if (testStatus == TestUsesDifferentSemantics)
        QSKIP("Test uses different semantics");

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

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

    if (testStatus == TestFails)
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    QVERIFY(runTest(parser.table(), testDescription.object()));
}

static QList<QByteArray> getStates(const QJsonObject &obj, const QString &key)
{
    QList<QByteArray> states;
    auto jsonStates = obj.value(key).toArray();
    for (int i = 0, ei = jsonStates.size(); i != ei; ++i) {
        QString state = jsonStates.at(i).toString();
        Q_ASSERT(!state.isEmpty());
        states.append(state.toUtf8());
    }
    std::sort(states.begin(), states.end());
    return states;
}

static bool verifyStates(StateTable *stateMachine, const QJsonObject &stateDescription, const QString &key, int counter)
{
    auto current = stateMachine->currentStates();
    std::sort(current.begin(), current.end());
    auto expected = getStates(stateDescription, key);
    if (current == expected)
        return true;

    qWarning("Incorrect %s (%d)!", qPrintable(key), counter);
    qWarning() << "Current configuration:" << current;
    qWarning() << "Expected configuration:" << expected;
    return false;
}

static bool playEvent(StateTable *stateMachine, const QJsonObject &eventDescription, int counter)
{
    if (!stateMachine->isRunning()) {
        qWarning() << "State machine stopped running!";
        return false;
    }

    Q_ASSERT(eventDescription.contains(QLatin1String("event")));
    auto event = eventDescription.value(QLatin1String("event")).toObject();
    auto eventName = event.value(QLatin1String("name")).toString().toUtf8();
    Q_ASSERT(!eventName.isEmpty());
    ScxmlEvent::EventType type = ScxmlEvent::External;
    if (event.contains(QLatin1String("type"))) {
        QString typeStr = event.value(QLatin1String("type")).toString();
        if (typeStr.compare(QLatin1String("external"), Qt::CaseInsensitive) == 0)
            type = ScxmlEvent::Internal;
        else if (typeStr.compare(QLatin1String("platform"), Qt::CaseInsensitive) == 0)
            type = ScxmlEvent::Platform;
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
    ScxmlEvent *e = new ScxmlEvent(eventName, type, dataValues, dataNames, sendid, origin, origintype, invokeid);
    if (eventDescription.contains(QLatin1String("after"))) {
        int delay = eventDescription.value(QLatin1String("after")).toInt();
        Q_ASSERT(delay > 0);
        stateMachine->submitDelayedEvent(delay, e);
    } else {
        stateMachine->submitEvent(e);
    }

    if (!MySignalSpy(stateMachine, SIGNAL(reachedStableState(bool))).wait()) {
        qWarning() << "State machine did not reach a stable state!";
    } else if (verifyStates(stateMachine, eventDescription, QLatin1String("nextConfiguration"), counter)) {
        return true;
    }

    qWarning() << "... after sending event" << event;
    return false;
}

static bool playEvents(StateTable *stateMachine, const QJsonObject &testDescription)
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

bool TestScion::runTest(StateTable *stateMachine, const QJsonObject &testDescription)
{
    MySignalSpy stableStateSpy(stateMachine, SIGNAL(reachedStableState(bool)));
    MySignalSpy finishedSpy(stateMachine, SIGNAL(finished()));

    QJSEngine *jsEngine = new QJSEngine(stateMachine);
    stateMachine->setEngine(jsEngine);
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

QTEST_MAIN(TestScion)
#include "tst_scion.moc"
