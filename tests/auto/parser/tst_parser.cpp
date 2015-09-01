/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <QObject>
#include <QXmlStreamReader>
#include <QtScxml/scxmlparser.h>
#include <QtScxml/scxmlstatemachine.h>

Q_DECLARE_METATYPE(Scxml::ScxmlError);

using namespace Scxml;

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
    QTest::addColumn<QVector<Scxml::ScxmlError> >("expectedErrors");

    QVector<Scxml::ScxmlError> errors;
    errors << ScxmlError(QString(":/tst_parser/test1.scxml"), 5, 46, QString("unknown state 'b' in target"));
    QTest::newRow("test1") << QString(":/tst_parser/test1.scxml") << errors;
}

void tst_Parser::error()
{
    QFETCH(QString, scxmlFileName);
    QFETCH(QVector<Scxml::ScxmlError>, expectedErrors);

    StateMachine *stateMachine = StateMachine::fromFile(scxmlFileName);

    QVector<Scxml::ScxmlError> errors = stateMachine->errors();
    QCOMPARE(errors.count(), expectedErrors.count());

    for (int i = 0; i < errors.count(); ++i) {
        ScxmlError error = errors.at(i);
        ScxmlError expectedError = expectedErrors.at(i);
        QCOMPARE(error.fileName(), expectedError.fileName());
        QCOMPARE(error.line(), expectedError.line());
        QCOMPARE(error.column(), expectedError.column());
        QCOMPARE(error.description(), expectedError.description());
    }
}

QTEST_MAIN(tst_Parser)

#include "tst_parser.moc"


