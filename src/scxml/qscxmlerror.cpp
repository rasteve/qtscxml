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

#include "qscxmlerror.h"

QT_BEGIN_NAMESPACE

class QScxmlError::ScxmlErrorPrivate
{
public:
    ScxmlErrorPrivate()
        : line(-1)
        , column(-1)
    {}

    QString fileName;
    int line;
    int column;
    QString description;
};

QScxmlError::QScxmlError()
    : d(Q_NULLPTR)
{}

QScxmlError::QScxmlError(const QString &fileName, int line, int column, const QString &description)
    : d(new ScxmlErrorPrivate)
{
    d->fileName = fileName;
    d->line = line;
    d->column = column;
    d->description = description;
}

QScxmlError::QScxmlError(const QScxmlError &other)
    : d(Q_NULLPTR)
{
    *this = other;
}

QScxmlError &QScxmlError::operator=(const QScxmlError &other)
{
    if (other.d) {
        if (!d)
            d = new ScxmlErrorPrivate;
        d->fileName = other.d->fileName;
        d->line = other.d->line;
        d->column = other.d->column;
        d->description = other.d->description;
    } else {
        delete d;
        d = Q_NULLPTR;
    }
    return *this;
}

QScxmlError::~QScxmlError()
{
    delete d;
    d = Q_NULLPTR;
}

bool QScxmlError::isValid() const
{
    return d != Q_NULLPTR;
}

QString QScxmlError::fileName() const
{
    return isValid() ? d->fileName : QString();
}

int QScxmlError::line() const
{
    return isValid() ? d->line : -1;
}

int QScxmlError::column() const
{
    return isValid() ? d->column : -1;
}

QString QScxmlError::description() const
{
    return isValid() ? d->description : QString();
}

QString QScxmlError::toString() const
{
    QString str;
    if (!isValid())
        return str;

    if (d->fileName.isEmpty())
        str = QStringLiteral("<Unknown File>");
    else
        str = d->fileName;
    if (d->line != -1) {
        str += QStringLiteral(":%1").arg(d->line);
        if (d->column != -1)
            str += QStringLiteral(":%1").arg(d->column);
    }
    str += QStringLiteral(": error: ") + d->description;

    return str;
}

QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QScxmlError &error)
{
    debug << error.toString();
    return debug;
}

QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<QScxmlError> &errors)
{
    foreach (const QScxmlError &error, errors) {
        debug << error << endl;
    }
    return debug;
}

QT_END_NAMESPACE
