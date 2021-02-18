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
#include <QtScxml/QScxmlNullDataModel>
#include <QtScxmlQml/private/eventconnection_p.h>
#include <QtScxmlQml/private/invokedservices_p.h>
#include <QtScxmlQml/private/statemachineloader_p.h>
#include "topmachine.h"
#include <functional>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <memory>

#include <QDebug>

class tst_scxmlqmlcpp: public QObject
{
    Q_OBJECT
private slots:
    void bindings();
private:

    // This is a helper function to test basics of typical bindable
    //  properties that are writable. Primarily ensure:
    // - properties work as before bindings
    // - added bindable aspects work
    //
    // "TestedClass" is the class type we are testing
    // "TestedData" is the data type of the property we are testing
    // "testedClass" is an instance of the class we are interested testing
    // "data1" and "data2" are two different instances of property data to set and get
    // "propertyName" is the name of the property we are interested in testing
    template<typename TestedClass, typename TestedData>
    void testWritableBindableBasics(TestedClass& testedClass, TestedData data1,
                            TestedData data2, const char* propertyName)
    {
        // Get the property we are testing
        const QMetaObject *metaObject = testedClass.metaObject();
        QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));

        // Generate a string to help identify failures (as this is a generic template)
        QString id(metaObject->className());
        id.append(QStringLiteral("::"));
        id.append(metaProperty.name());

        // Fail gracefully if preconditions to use this helper function are not met:
        QVERIFY2(metaProperty.isBindable() && metaProperty.isWritable()
                && metaProperty.hasNotifySignal(), qPrintable(id));
        // Create a signal spy for the property changed -signal
        QSignalSpy spy(&testedClass, metaProperty.notifySignal());
        QUntypedBindable bindable = metaProperty.bindable(&testedClass);

        // Test basic property read and write
        testedClass.setProperty(propertyName, QVariant::fromValue(data1));
        QVERIFY2(testedClass.property(propertyName).template value<TestedData>() == data1, qPrintable(id));
        QVERIFY2(spy.count() == 1, qPrintable(id + ", actual: " + QString::number(spy.count())));

        // Test setting a binding as a source for the property
        QProperty<TestedData> property1(data1);
        QProperty<TestedData> property2(data2);
        QVERIFY2(!bindable.hasBinding(), qPrintable(id));
        bindable.setBinding(Qt::makePropertyBinding(property2));
        QVERIFY2(bindable.hasBinding(), qPrintable(id));
        // Check that the value also changed
        QVERIFY2(testedClass.property(propertyName).template value<TestedData>() == data2, qPrintable(id));
        QVERIFY2(spy.count() == 2, qPrintable(id + ", actual: " + QString::number(spy.count())));
        // Same test but with a lambda binding (cast to be able to set the lambda directly)
        QBindable<TestedData> *typedBindable = static_cast<QBindable<TestedData>*>(&bindable);
        typedBindable->setBinding([&](){ return property1.value(); });
        QVERIFY2(typedBindable->hasBinding(), qPrintable(id));
        QVERIFY2(testedClass.property(propertyName).template value<TestedData>() == data1, qPrintable(id));
        QVERIFY2(spy.count() == 3, qPrintable(id + ", actual: " + QString::number(spy.count())));

        // Remove binding by setting a value directly
        QVERIFY2(bindable.hasBinding(), qPrintable(id));
        testedClass.setProperty(propertyName, QVariant::fromValue(data2));
        QVERIFY2(testedClass.property(propertyName).template value<TestedData>() == data2, qPrintable(id));
        QVERIFY2(!bindable.hasBinding(), qPrintable(id));
        QVERIFY2(spy.count() == 4, qPrintable(id + ", actual: " + QString::number(spy.count())));

        // Test using the property as the source in a binding
        QProperty<bool> data1Used([&](){
            return testedClass.property(propertyName).template value<TestedData>() == data1;
        });
        QVERIFY2(data1Used == false, qPrintable(id));
        testedClass.setProperty(propertyName, QVariant::fromValue(data1));
        QVERIFY2(data1Used == true, qPrintable(id));
    }
};

void tst_scxmlqmlcpp::bindings() {

    // -- test eventconnection::stateMachine
    QScxmlEventConnection eventConnection;
    std::unique_ptr<QScxmlStateMachine> sm1(QScxmlStateMachine::fromFile("no_real_file_needed"));
    std::unique_ptr<QScxmlStateMachine> sm2(QScxmlStateMachine::fromFile("no_real_file_needed"));
    testWritableBindableBasics<QScxmlEventConnection, QScxmlStateMachine*>(
                eventConnection, sm1.get(), sm2.get(), "stateMachine");

    // -- test eventconnection::events
    QStringList eventList1{{"event1"},{"event2"}};
    QStringList eventList2{{"event3"},{"event4"}};
    testWritableBindableBasics<QScxmlEventConnection, QStringList>(
                eventConnection, eventList1, eventList2, "events");

    // -- test invokedservices::statemachine
    QScxmlInvokedServices invokedServices;
    testWritableBindableBasics<QScxmlInvokedServices, QScxmlStateMachine*>(
                invokedServices, sm1.get(), sm2.get(), "stateMachine");

    // -- test invokedservices::children
    TopMachine topSm;
    invokedServices.setStateMachine(&topSm);
    QCOMPARE(invokedServices.children().count(), 0);
    QCOMPARE(topSm.invokedServices().count(), 0);

    // at some point during the topSm execution there are 3 invoked services
    // of the same name ('3' filters out as '1' at QML binding)
    topSm.start();
    QTRY_COMPARE(topSm.invokedServices().count(), 3);
    QCOMPARE(invokedServices.children().count(), 1);

    // after completion invoked services drop back to 0
    QTRY_COMPARE(topSm.invokedServices().count(), 0);
    QCOMPARE(invokedServices.children().count(), 0);

    // bind *to* the invokedservices property and check that we observe same changes
    // during the topSm execution
    QProperty<qsizetype> serviceCounter;
    serviceCounter.setBinding([&](){ return invokedServices.children().count(); });

    QCOMPARE(serviceCounter, 0);
    topSm.start();
    QTRY_COMPARE(serviceCounter, 1);
    QCOMPARE(topSm.invokedServices().count(), 3);

    // -- test statemachineloader::initialValues
    QScxmlStateMachineLoader stateMachineLoader;
    QVariantMap values1{{"key1","value1"}, {"key2","value2"}};
    QVariantMap values2{{"key3","value3"}, {"key4","value4"}};
    testWritableBindableBasics<QScxmlStateMachineLoader, QVariantMap>(
                stateMachineLoader, values1, values2, "initialValues");

    // -- test statemachineloader::source
    QUrl source1(QStringLiteral("qrc:///statemachine.scxml"));
    QUrl source2(QStringLiteral("qrc:///topmachine.scxml"));
    // The 'setSource' assumes a valid qml context, so we need to create a bit differently
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\n; import QtScxml\n;  Item { StateMachineLoader { objectName: \'sml\'; } }", QUrl());
    std::unique_ptr<QObject> root(component.create());
    QScxmlStateMachineLoader *sml = qobject_cast<QScxmlStateMachineLoader*>(root->findChild<QObject*>("sml"));
    QVERIFY(sml != nullptr);
    testWritableBindableBasics<QScxmlStateMachineLoader, QUrl>(*sml, source1, source2, "source");

    // -- test statemachineloader::datamodel
    QScxmlNullDataModel model1;
    QScxmlNullDataModel model2;
    testWritableBindableBasics<QScxmlStateMachineLoader,QScxmlDataModel*>
            (stateMachineLoader, &model1, &model2, "dataModel");

    // -- test statemachineloader::statemachine
    // The statemachine can be indirectly set by setting the source
    QSignalSpy smSpy(sml, &QScxmlStateMachineLoader::stateMachineChanged);
    QUrl sourceNonexistent(QStringLiteral("qrc:///file_doesnt_exist.scxml"));
    QUrl sourceBroken(QStringLiteral("qrc:///brokenstatemachine.scxml"));

    QVERIFY(sml->stateMachine() != nullptr);
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:11: QML StateMachineLoader: Cannot open 'qrc:///file_doesnt_exist.scxml' for reading.");
    sml->setSource(sourceNonexistent);
    QVERIFY(sml->stateMachine() == nullptr);
    QCOMPARE(smSpy.count(), 1);
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:11: QML StateMachineLoader: :/brokenstatemachine.scxml:59:1: error: initial state 'working' not found for <scxml> element");
    QTest::ignoreMessage(QtWarningMsg,
                        "SCXML document has errors");
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:11: QML StateMachineLoader: Something went wrong while parsing 'qrc:///brokenstatemachine.scxml':\n");
    sml->setSource(sourceBroken);
    QVERIFY(sml->stateMachine() == nullptr);
    QCOMPARE(smSpy.count(), 1);

    QProperty<bool> hasStateMachine([&](){ return sml->stateMachine() ? true : false; });
    QVERIFY(hasStateMachine == false);
    sml->setSource(source1);
    QCOMPARE(smSpy.count(), 2);
    QVERIFY(hasStateMachine == true);
}

QTEST_MAIN(tst_scxmlqmlcpp)
#include "tst_scxmlqmlcpp.moc"
