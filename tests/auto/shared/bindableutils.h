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

#ifndef BINDABLEUTILS_H
#define BINDABLEUTILS_H

#include <QtTest>
#include <QObject>

// This is a helper function to test basics of typical bindable
//  properties that are writable. Primarily ensure:
// - properties work as before bindings
// - added bindable aspects work
//
// "TestedClass" is the class type we are testing
// "TestedData" is the data type of the property we are testing
// "testedClass" is an instance of the class we are interested testing
// "data1" and "data2" are two different instances of property data to set and get
// The "data1" and "data2" must differ from one another, and
// the "data1" must differ from instance property's initial state
// "propertyName" is the name of the property we are interested in testing
template<typename TestedClass, typename TestedData>
void testWritableBindableBasics(TestedClass& testedClass, TestedData data1,
                        TestedData data2, const char* propertyName,
                        std::function<bool(TestedData,TestedData)> dataComparator = [](TestedData d1, TestedData d2) { return d1 == d2; })
{
    // Get the property we are testing
    const QMetaObject *metaObject = testedClass.metaObject();
    QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));

    // Generate a string to help identify failures (as this is a generic template)
    QString id(metaObject->className());
    id.append(QStringLiteral("::"));
    id.append(propertyName);

    // Fail gracefully if preconditions to use this helper function are not met:
    QVERIFY2(metaProperty.isBindable() && metaProperty.isWritable()
            && metaProperty.hasNotifySignal(), qPrintable(id));
    // Create a signal spy for the property changed -signal
    QSignalSpy spy(&testedClass, metaProperty.notifySignal());
    QUntypedBindable bindable = metaProperty.bindable(&testedClass);

    // Test basic property read and write
    testedClass.setProperty(propertyName, QVariant::fromValue(data1));

    QVERIFY2(dataComparator(testedClass.property(propertyName).template value<TestedData>(), data1), qPrintable(id));
    QVERIFY2(spy.count() == 1, qPrintable(id + ", actual: " + QString::number(spy.count())));

    // Test setting a binding as a source for the property
    QProperty<TestedData> property1(data1);
    QProperty<TestedData> property2(data2);
    QVERIFY2(!bindable.hasBinding(), qPrintable(id));
    bindable.setBinding(Qt::makePropertyBinding(property2));
    QVERIFY2(bindable.hasBinding(), qPrintable(id));
    // Check that the value also changed
    QVERIFY2(dataComparator(testedClass.property(propertyName).template value<TestedData>(), data2), qPrintable(id));
    QVERIFY2(spy.count() == 2, qPrintable(id + ", actual: " + QString::number(spy.count())));
    // Same test but with a lambda binding (cast to be able to set the lambda directly)
    QBindable<TestedData> *typedBindable = static_cast<QBindable<TestedData>*>(&bindable);
    typedBindable->setBinding([&](){ return property1.value(); });
    QVERIFY2(typedBindable->hasBinding(), qPrintable(id));
    QVERIFY2(dataComparator(testedClass.property(propertyName).template value<TestedData>(), data1), qPrintable(id));
    QVERIFY2(spy.count() == 3, qPrintable(id + ", actual: " + QString::number(spy.count())));

    // Remove binding by setting a value directly
    QVERIFY2(bindable.hasBinding(), qPrintable(id));
    testedClass.setProperty(propertyName, QVariant::fromValue(data2));
    QVERIFY2(dataComparator(testedClass.property(propertyName).template value<TestedData>(), data2), qPrintable(id));
    QVERIFY2(!bindable.hasBinding(), qPrintable(id));
    QVERIFY2(spy.count() == 4, qPrintable(id + ", actual: " + QString::number(spy.count())));

    // Test using the property as the source in a binding
    QProperty<bool> data1Used([&](){
        return dataComparator(testedClass.property(propertyName).template value<TestedData>(), data1);
    });
    QVERIFY2(data1Used == false, qPrintable(id));
    testedClass.setProperty(propertyName, QVariant::fromValue(data1));
    QVERIFY2(data1Used == true, qPrintable(id));
}

#endif // BINDABLEUTILS_H
