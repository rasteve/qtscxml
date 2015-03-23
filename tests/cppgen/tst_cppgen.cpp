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

#include <QScxmlLib/scxmlcppdumper.h>

using namespace Scxml;

class TestCppGen: public QObject
{
    Q_OBJECT

private slots:
    void idMangling_data();
    void idMangling();
};

void TestCppGen::idMangling_data()
{
    QTest::addColumn<QString>("unmangled");
    QTest::addColumn<QString>("mangled");

    QTest::newRow("One:Two") << "One:Two" << "One_colon_Two";
    QTest::newRow("one-piece") << "one-piece" << "one_dash_piece";
    QTest::newRow("two_words") << "two_words" << "two__words";
    QTest::newRow("me@work") << "me@work" << "me_at_work";
}

void TestCppGen::idMangling()
{
    QFETCH(QString, unmangled);
    QFETCH(QString, mangled);

    QCOMPARE(CppDumper::mangleId(unmangled), mangled);
}

QTEST_MAIN(TestCppGen)
#include "tst_cppgen.moc"
