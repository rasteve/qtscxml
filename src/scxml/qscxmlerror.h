/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScxml module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

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
