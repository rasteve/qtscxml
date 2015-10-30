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

/*!
 * \class QScxmlError
 * \brief Describes the errors returned by the QScxmlStateMachine when parsing an Scxml file.
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

/*!
 * \brief Destroys a QScxmlError.
 */
QScxmlError::~QScxmlError()
{
    delete d;
    d = Q_NULLPTR;
}

/*!
 * \return Returns true when the error is valid, false otherwise. An invalid error can only be
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

/*!
 * \brief Convenience method to write a QScxmlError to a QDebug stream.
 */
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QScxmlError &error)
{
    debug << error.toString();
    return debug;
}

/*!
 * \brief Convenience method to write QScxmlErrors to a QDebug stream.
 */
QDebug Q_SCXML_EXPORT operator<<(QDebug debug, const QVector<QScxmlError> &errors)
{
    foreach (const QScxmlError &error, errors) {
        debug << error << endl;
    }
    return debug;
}

QT_END_NAMESPACE
