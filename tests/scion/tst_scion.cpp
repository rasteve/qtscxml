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

#include <qscxmllib/scxmlparser.h>

#include "../3rdparty/scion.h"

enum { SpyWaitTime = 5000 };

static QSet<QString> weFailOnThese = QSet<QString>()
        ;

static QSet<QString> weDieOnThese = QSet<QString>()
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send1")
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send2")
        << QLatin1String("scion-tests/scxml-test-framework/test/delayedSend/send3")
        << QLatin1String("scion-tests/scxml-test-framework/test/foreach/test1")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history3") // infinite loop?
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history5") // infinite loop?
        << QLatin1String("scion-tests/scxml-test-framework/test/send-data/send1")
        << QLatin1String("scion-tests/scxml-test-framework/test/send-internal/test0")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test144.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test147.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test150.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test151.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test152.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test153.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test155.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test156.txml")
           // the ones below here require <send> to work
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test187.txml") // sub state machine?
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test201.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test205.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test205.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test207.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test208.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test208.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test210.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test210.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test215.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test216.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test220.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test223.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test224.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test225.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test226.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test228.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test229.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test229.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test229.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test229.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test230.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test230.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test230.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test232.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test232.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test232.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test233.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test233.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test234.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test234.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test234.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test235.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test236.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test236.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test237.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test237.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test237.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test239.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test240.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test241.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test242.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test242.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test242.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test243.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test243.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test243.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test244.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test244.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test244.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test245.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test245.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test245.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test247.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test250.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test250.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test252.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test252.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test252.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test252.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test253.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test330.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test331.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test332.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test333.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test336.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test336.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test336.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test338.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test338.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test342.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test364.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test372.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test376.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test378.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test387.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test388.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test399.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test401.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test402.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test403a.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test403c.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test405.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test406.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test409.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test411.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test412.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test416.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test417.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test419.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test421.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test422.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test422.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test422.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test422.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test423.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test423.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test521.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test530.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test554.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test560.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test562.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test570.txml")
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test576.txml")
           // <foreach>
        << QLatin1String("scion-tests/scxml-test-framework/test/w3c-ecma/test525.txml")
           ;

static QSet<QString> differentSemantics = QSet<QString>()
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history0")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history1")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history2")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history4")
        << QLatin1String("scion-tests/scxml-test-framework/test/history/history6")
        ;

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

void TestScion::scion_data()
{
    QTest::addColumn<QString>("scxml");
    QTest::addColumn<QString>("json");
    QTest::addColumn<bool>("weFailOnThis");

    const int nrOfTests = sizeof(testBases)/sizeof(const char *);
    for (int i = 0; i < nrOfTests; ++i) {
        QString base = QString::fromUtf8(testBases[i]);
        if (weDieOnThese.contains(base) || differentSemantics.contains(base))
            continue;
        QTest::newRow(testBases[i]) << base + QLatin1String(".scxml")
                                    << base + QLatin1String(".json")
                                    << weFailOnThese.contains(base);
    }
}

void TestScion::scion()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(bool, weFailOnThis);

    fprintf(stderr, "\n\n%s\n\n", qPrintable(json));

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

    QFile scxmlFile(QLatin1String(":/") + scxml);
    QVERIFY(scxmlFile.open(QIODevice::ReadOnly));
    QXmlStreamReader xmlReader(&scxmlFile);
    ScxmlParser parser(&xmlReader);
    parser.parse();
    QCOMPARE(parser.state(), ScxmlParser::FinishedParsing);
    QVERIFY(parser.errors().isEmpty());
    scxmlFile.close();

    if (weFailOnThis)
        QEXPECT_FAIL("", "We are expected to fail", Continue);
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

static bool verifyStates(StateTable *stateMachine, const QJsonObject &stateDescription, const QString &key)
{
    auto current = stateMachine->currentStates();
    std::sort(current.begin(), current.end());
    auto expected = getStates(stateDescription, key);
    if (current == expected)
        return true;

    qWarning() << "Incorrect" << key << "!";
    qWarning() << "Current configuration:" << current;
    qWarning() << "Expected configuration:" << expected;
    return false;
}

static bool playEvent(StateTable *stateMachine, const QJsonObject &eventDescription)
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
    QVariantList datas;
    QStringList dataNames;
    // remove ifs and rely on defaults?
    if (event.contains(QLatin1String("data"))) {
        QJsonValue dataVal = event.value(QLatin1String("data"));
        if (dataVal.isObject()) {
            QJsonObject dataObj = dataVal.toObject();
            for (QJsonObject::const_iterator i = dataObj.constBegin(); i != dataObj.constEnd(); ++i) {
                dataNames.append(i.key());
                datas.append(i.value().toVariant());
            }
        } else {
            datas.append(dataVal.toVariant());
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
    stateMachine->submitEvent(eventName, datas, dataNames, type, sendid, origin, origintype, invokeid);

    if (!QSignalSpy(stateMachine, SIGNAL(reachedStableState(bool))).wait(SpyWaitTime)) {
        qWarning() << "State machine did not reach a stable state!";
    } else if (verifyStates(stateMachine, eventDescription, QLatin1String("nextConfiguration")))
        return true;

    qWarning() << "... after sending event" << event;
    return false;
}

static bool playEvents(StateTable *stateMachine, const QJsonObject &testDescription)
{
    auto jsonEvents = testDescription.value(QLatin1String("events"));
    Q_ASSERT(!jsonEvents.isNull());
    auto eventsArray = jsonEvents.toArray();
    for (int i = 0, ei = eventsArray.size(); i != ei; ++i) {
        if (!playEvent(stateMachine, eventsArray.at(i).toObject()))
            return false;
    }
    return true;
}

bool TestScion::runTest(StateTable *stateMachine, const QJsonObject &testDescription)
{
    QSignalSpy stableStateSpy(stateMachine, SIGNAL(reachedStableState(bool)));
    QSignalSpy finishedSpy(stateMachine, SIGNAL(finished()));

    QJSEngine *jsEngine = new QJSEngine(stateMachine);
    stateMachine->setEngine(jsEngine);
    if (!stateMachine->init()) {
        qWarning() << "init failed";
        return false;
    }
    stateMachine->start();

    if (testDescription.contains(QLatin1String("events"))
            && !testDescription.value(QLatin1String("events")).toArray().isEmpty()) {
        if (!stableStateSpy.wait(SpyWaitTime)) {
            qWarning() << "Failed to reach stable initial state!";
            return false;
        }

        if (!verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration")))
            return false;

        return playEvents(stateMachine, testDescription);
    } else {
        if (!finishedSpy.wait(SpyWaitTime)) {
            qWarning() << "Failed to reach final state!";
            return false;
        }
        return verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration"));
    }
}

QTEST_MAIN(TestScion)
#include "tst_scion.moc"
