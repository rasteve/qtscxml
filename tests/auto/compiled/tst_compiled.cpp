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
#include "ids1.h"
#include "statemachineunicodename.h"

Q_DECLARE_METATYPE(QScxmlError);

enum { SpyWaitTime = 8000 };

class tst_Compiled: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stateNames();
};

void tst_Compiled::stateNames()
{
    ids1 stateMachine;

    QStringList ids1States({
        "_",
        "_VALID",
        "__valid",
        "foo-bar",
        "foo.bar",
        "foo_bar",
        "n_0xe4_l",
        "näl",
        "qÿ̀i",
    });

    QCOMPARE(stateMachine.stateNames(false), ids1States);

    foreach (const QString &state, ids1States) {
        QVariant prop = stateMachine.property(state.toUtf8().constData());
        QVERIFY(!prop.isNull());
        QVERIFY(prop.isValid());
        QCOMPARE(prop.toBool(), false);
    }

    QVariant invalidProp = stateMachine.property("blabla");
    QVERIFY(invalidProp.isNull());
    QVERIFY(!invalidProp.isValid());

    QStringList calculatorStates(QLatin1String("wrapper"));

    Calculator_0xe4_tateMachine stateMachine3;
    QCOMPARE(stateMachine3.stateNames(false), calculatorStates);
}

QTEST_MAIN(tst_Compiled)

#include "tst_compiled.moc"


