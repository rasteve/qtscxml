/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QObject>
#include <QtScxml/QScxmlStateMachine>
#include <QtScxmlQml/private/eventconnection_p.h>
#include <memory>

#include <QDebug>

class tst_scxmlqmlcpp: public QObject
{
    Q_OBJECT
private slots:
    void bindings();
};

void tst_scxmlqmlcpp::bindings() {

    // First create some statemachine instances
    std::unique_ptr<QScxmlStateMachine> sm0(nullptr);
    std::unique_ptr<QScxmlStateMachine> sm1(QScxmlStateMachine::fromFile("no_real_file_needed"));
    std::unique_ptr<QScxmlStateMachine> sm2(QScxmlStateMachine::fromFile("no_real_file_needed"));

    // -- test eventconnection::stateMachine
    QScxmlEventConnection eventConnection;
    QSignalSpy ecStateMachineSpy(&eventConnection, &QScxmlEventConnection::stateMachineChanged);

    // initially no state machine
    QVERIFY(eventConnection.bindableStateMachine().value() == nullptr);
    QCOMPARE(ecStateMachineSpy.count(), 0);

    // access state machine through getter/setter
    eventConnection.setStateMachine(sm1.get());
    QVERIFY(eventConnection.bindableStateMachine().value() == sm1.get());
    QVERIFY(eventConnection.stateMachine() == sm1.get());
    QCOMPARE(ecStateMachineSpy.count(), 1);

    // access state machine through properties
    eventConnection.setProperty("stateMachine", QVariant::fromValue(sm2.get()));
    QVERIFY(eventConnection.bindableStateMachine().value() == sm2.get());
    QVERIFY(eventConnection.property("stateMachine").value<QScxmlStateMachine*>() == sm2.get());
    QCOMPARE(ecStateMachineSpy.count(), 2);

    // set state machine through property binding
    QProperty<QScxmlStateMachine*> smp1;
    eventConnection.bindableStateMachine().setBinding(Qt::makePropertyBinding(smp1));
    QVERIFY(eventConnection.stateMachine() == nullptr);
    QCOMPARE(ecStateMachineSpy.count(), 3);

    // set state machine through lambda property binding
    QProperty<QScxmlStateMachine*> smp2(sm2.get());
    eventConnection.bindableStateMachine().setBinding([&](){ return smp2.value(); });
    QVERIFY(eventConnection.stateMachine() == sm2.get());
    QCOMPARE(ecStateMachineSpy.count(), 4);

    // remove binding by setting a value directly
    QVERIFY(eventConnection.bindableStateMachine().hasBinding());
    eventConnection.setStateMachine(nullptr);
    QVERIFY(!eventConnection.bindableStateMachine().hasBinding());

    // bind *to* the stateMachine property
    QProperty<bool> hasMachineProperty(false);
    QVERIFY(!hasMachineProperty.value()); // initially no state machine
    hasMachineProperty.setBinding([&](){ return eventConnection.stateMachine() ? true : false; });
    QVERIFY(!hasMachineProperty.value()); // still no state machine
    eventConnection.setStateMachine(sm1.get());
    QVERIFY(hasMachineProperty.value()); // a statemachine via binding

    // -- test eventconnection::events
    QStringList eventList1{{"event1"},{"event2"}};
    QStringList eventList2{{"event3"},{"event4"}};
    QSignalSpy ecEventsSpy(&eventConnection, &QScxmlEventConnection::eventsChanged);
    QCOMPARE(eventConnection.events().count(), 0);

    // setter / getter access
    eventConnection.setEvents(eventList1);
    QCOMPARE(eventConnection.bindableEvents().value(), eventList1);
    QCOMPARE(eventConnection.events(), eventList1);
    QCOMPARE(ecEventsSpy.count(), 1);

    // property access
    eventConnection.setProperty("events", eventList2);
    QCOMPARE(eventConnection.bindableEvents().value(), eventList2);
    QCOMPARE(eventConnection.property("events"), eventList2);
    QCOMPARE(ecEventsSpy.count(), 2);

    // property binding updates
    QProperty<bool> toggle(true);
    QProperty<QStringList> evp1([&](){ return toggle ? eventList1 : eventList2;});
    eventConnection.bindableEvents().setBinding(Qt::makePropertyBinding(evp1));
    QCOMPARE(eventConnection.events(), eventList1);
    toggle = false;
    QCOMPARE(eventConnection.events(), eventList2);
    QCOMPARE(ecEventsSpy.count(), 4);

    // remove binding by setting value
    QVERIFY(eventConnection.bindableEvents().hasBinding());
    eventConnection.setEvents(eventList1);
    QVERIFY(!eventConnection.bindableEvents().hasBinding());
    QCOMPARE(ecEventsSpy.count(), 5);

    // bind *to* the events property
    QProperty<bool> hasEvents([&](){ return !eventConnection.events().isEmpty(); });
    QVERIFY(hasEvents);
    eventConnection.setEvents({});
    QVERIFY(!hasEvents);
    QCOMPARE(ecEventsSpy.count(), 6);
}

QTEST_MAIN(tst_scxmlqmlcpp)
#include "tst_scxmlqmlcpp.moc"
