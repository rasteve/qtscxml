/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
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

/*!
 * \class QScxmlError
 * \brief Describes the errors returned by the QScxmlStateMachine when parsing an SCXML file.
 * \since 5.6
 * \inmodule QtScxml
 *
 * \sa QScxmlStateMachine QScxmlParser
 */

/*!
 * \brief Creates a new invalid QScxmlError.
 */
QScxmlError::QScxmlError()
    : d(Q_NULLPTR)
{}

/*!
 * \brief Creates a new (valid) QScxmlError.
 *
 * \param fileName The name of the file in which the error occurred.
 * \param line The line on which the error occurred.
 * \param column The column in which the error occurred.
 * \param description The error message.
 */
QScxmlError::QScxmlError(const QString &fileName, int line, int column, const QString &description)
    : d(new ScxmlErrorPrivate)
{
    d->fileName = fileName;
    d->line = line;
    d->column = column;
    d->description = description;
}

/*!
 * \brief Copies a QScxmlError.
 */
QScxmlError::QScxmlError(const QScxmlError &other)
    : d(Q_NULLPTR)
{
    *this = other;
}

/*!
 * \brief Copies a QScxmlError.
 */
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

/*!
 * \brief Destroys a QScxmlError.
 */
QScxmlError::~QScxmlError()
{
    delete d;
    d = Q_NULLPTR;
}

/*!
 * \return true when the error is valid, false otherwise. An invalid error can only be
 *         created by calling the default constructor, or by assigning an invalid error.
 */
bool QScxmlError::isValid() const
{
    return d != Q_NULLPTR;
}

/*!
 * \return The name of the file in which the error occurred.
 */
QString QScxmlError::fileName() const
{
    return isValid() ? d->fileName : QString();
}

/*!
 * \return The line on which the error occurred.
 */
int QScxmlError::line() const
{
    return isValid() ? d->line : -1;
}

/*!
 * \return The column in which the error occurred.
 */
int QScxmlError::column() const
{
    return isValid() ? d->column : -1;
}

/*!
 * \return The error message.
 */
QString QScxmlError::description() const
{
    return isValid() ? d->description : QString();
}

/*!
 * \brief Convenience method to convert an error to a string.
 * \return The error message formatted as: "filename:line:column: error: description"
 */
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

QT_END_NAMESPACE
