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

#ifndef QSCXMLERROR_H
#define QSCXMLERROR_H

#include <QtScxml/qscxmlglobals.h>

#include <QString>

QT_BEGIN_NAMESPACE

class Q_SCXML_EXPORT QScxmlError
{
public:
    QScxmlError();
    QScxmlError(const QString &fileName, int line, int column, const QString &description);
    QScxmlError(const QScxmlError &);
    QScxmlError &operator=(const QScxmlError &);
    ~QScxmlError();

    bool isValid() const;

    QString fileName() const;
    int line() const;
    int column() const;
    QString description() const;

    QString toString() const;

private:
    class ScxmlErrorPrivate;
    ScxmlErrorPrivate *d;
};

class QDebug;
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QScxmlError &error);
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<QScxmlError> &errors);

QT_END_NAMESPACE

#endif // QSCXMLERROR_H
