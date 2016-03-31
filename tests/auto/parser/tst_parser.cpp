/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QXmlStreamReader>
#include <QtScxml/qscxmlparser.h>
#include <QtScxml/qscxmlstatemachine.h>

Q_DECLARE_METATYPE(QScxmlError);

class tst_Parser: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void error_data();
    void error();
};

void tst_Parser::error_data()
{
    QTest::addColumn<QString>("scxmlFileName");
    QTest::addColumn<QVector<QScxmlError> >("expectedErrors");

    QVector<QScxmlError> errors;
    errors << QScxmlError(QString(":/tst_parser/test1.scxml"), 34, 46,
                          QString("unknown state 'b' in target"));
    QTest::newRow("test1") << QString(":/tst_parser/test1.scxml") << errors;

    QTest::newRow("namespaces 1") << QStringLiteral(":/tst_parser/namespaces1.scxml")
                                  << QVector<QScxmlError>();
    QTest::newRow("IDs 1") << QStringLiteral(":/tst_parser/ids1.scxml") << QVector<QScxmlError>();
    QTest::newRow("IDs 2") << QStringLiteral(":/tst_parser/ids2.scxml") << (QVector<QScxmlError>()
        << QScxmlError(":/tst_parser/ids2.scxml", 33, 25,
                    "state id 'foo.bar' is not a valid C++ identifier in Qt mode")
        << QScxmlError(":/tst_parser/ids2.scxml", 34, 25,
                    "state id 'foo-bar' is not a valid C++ identifier in Qt mode")
        << QScxmlError(":/tst_parser/ids2.scxml", 36, 19, "'1' is not a valid XML ID")
    );
    QTest::newRow("eventnames") << QStringLiteral(":/tst_parser/eventnames.scxml")
                                << (QVector<QScxmlError>()
        << QScxmlError(":/tst_parser/eventnames.scxml", 50, 38, "'.invalid' is not a valid event")
        << QScxmlError(":/tst_parser/eventnames.scxml", 51, 38, "'invalid.' is not a valid event")
        << QScxmlError(":/tst_parser/eventnames.scxml", 39, 36, "'.invalid' is not a valid event")
        << QScxmlError(":/tst_parser/eventnames.scxml", 40, 36, "'invalid.' is not a valid event")
        << QScxmlError(":/tst_parser/eventnames.scxml", 41, 36, "'in valid' is not a valid event")
    );
}

void tst_Parser::error()
{
    QFETCH(QString, scxmlFileName);
    QFETCH(QVector<QScxmlError>, expectedErrors);

    QScopedPointer<QScxmlStateMachine> stateMachine(QScxmlStateMachine::fromFile(scxmlFileName));
    QVERIFY(!stateMachine.isNull());

    QVector<QScxmlError> errors = stateMachine->parseErrors();
    if (errors.count() != expectedErrors.count()) {
        foreach (const QScxmlError &error, errors) {
            qDebug() << error.toString();
        }
    }
    QCOMPARE(errors.count(), expectedErrors.count());

    for (int i = 0; i < errors.count(); ++i) {
        QScxmlError error = errors.at(i);
        QScxmlError expectedError = expectedErrors.at(i);
        QCOMPARE(error.fileName(), expectedError.fileName());
        QCOMPARE(error.line(), expectedError.line());
        QCOMPARE(error.column(), expectedError.column());
        QCOMPARE(error.description(), expectedError.description());
    }
}

QTEST_MAIN(tst_Parser)

#include "tst_parser.moc"


