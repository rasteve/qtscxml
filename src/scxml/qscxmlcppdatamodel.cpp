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

#include "qscxmlcppdatamodel_p.h"
#include "qscxmlstatemachine.h"

QT_BEGIN_NAMESPACE

using namespace QScxmlExecutableContent;

/*!
   \class QScxmlCppDataModel
   \brief The QScxmlCppDataModel class is a C++ data model for a Qt SCXML state machine.
   \since 5.7
   \inmodule QtScxml

   \sa QScxmlStateMachine QScxmlDataModel

   The C++ data model for SCXML that lets you write C++ code for \e expr attributes and \c <script>
   elements. The \e {data part} of the data model is backed by a subclass of QScxmlCppDataModel, for
   which the Qt SCXML compiler (\c qscxmlc) will generate the dispatch methods. It cannot be used
   when loading an SCXML file at runtime.

   Usage is through the \e datamodel attribute of the \c <scxml> element:
   \code
   <scxml datamodel="cplusplus:TheDataModel:thedatamodel.h"  ....>
   \endcode
   The format of the \e datamodel attribute is: \c{cplusplus:<class-name>:<classdef-header>}.
   So, for the example above, there should be a file \e thedatamodel.h containing a subclass of
   QScxmlCppDataModel, containing at least the following:
   \code
#include "qscxmlcppdatamodel.h"

class TheDataModel: public QScxmlCppDataModel
{
    Q_SCXML_DATAMODEL
};
   \endcode
   The Q_SCXML_DATAMODEL has to appear in the private section of the class definition, for example
   right after the opening bracket, or after a Q_OBJECT macro.
   This macro expands to the declaration of some virtual
   methods whose implementation is generated by the Qt SCXML compiler.

   \note You can of course inherit from both QScxmlCppDataModel and QObject.

   The Qt SCXML compiler will generate the various \c evaluateTo methods, and convert expressions and
   scripts into lambdas inside those methods. For example:
   \code
<scxml datamodel="cplusplus:TheDataModel:thedatamodel.h" xmlns="http://www.w3.org/2005/07/scxml" version="1.0" name="MediaPlayerStateMachine">
    <state id="stopped">
        <transition event="tap" cond="isValidMedia()" target="playing"/>
    </state>

    <state id="playing">
        <onentry>
            <script>
                media = eventData().value(QStringLiteral(&quot;media&quot;)).toString();
            </script>
            <send event="playbackStarted">
                <param name="media" expr="media"/>
            </send>
        </onentry>
    </state>
</scxml>
   \endcode
   This will result in:
   \code
bool TheDataModel::evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok) {
    // ....
        return [this]()->bool{ return isValidMedia(); }();
    // ....
}

QVariant TheDataModel::evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok) {
    // ....
        return [this]()->QVariant{ return media; }();
    // ....
}

void TheDataModel::evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok) {
    // ....
        [this]()->void{ media = eventData().value(QStringLiteral("media")).toString(); }();
    // ....
}
   \endcode

   So, you are not limited to call functions. In a \c <script> element you can put zero or more C++
   statements, and in \e cond or \e expr attributes you can use any C++ expression that can be
   converted to the respective bool or QVariant. And, as the \c this pointer is also captured, you
   can call or access the data model (the \e media attribute in the example above). For the full
   example, see \l {Qt SCXML: Media Player QML Example (C++ Data Model)}.
 */

/*!
 * Creates a new C++ data model with the parent object \a parent.
 */
QScxmlCppDataModel::QScxmlCppDataModel(QObject *parent)
    : QScxmlDataModel(*(new QScxmlCppDataModelPrivate), parent)
{}

/*! \internal */
QScxmlCppDataModel::~QScxmlCppDataModel()
{
}

/*!
 * Called during state machine initialization to set up a state machine using the initial values
 * for data model variables specified by their keys, \a initialDataValues. These
 * are the values specified by \c <param> tags in an \c <invoke> element.
 *
 * \sa QScxmlStateMachine::init
 */
bool QScxmlCppDataModel::setup(const QVariantMap &initialDataValues)
{
    Q_UNUSED(initialDataValues);

    return true;
}

void QScxmlCppDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    *ok = false;
    Q_UNREACHABLE();
}

void QScxmlCppDataModel::evaluateInitialization(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    *ok = false;
    Q_UNREACHABLE();
}

void QScxmlCppDataModel::evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_UNUSED(id);
    Q_UNUSED(body);
    *ok = false;
    Q_UNREACHABLE();
}

/*!
 * Sets the \a event that will be processed next.
 *
 * \sa QScxmlCppDataModel::scxmlEvent
 */
void QScxmlCppDataModel::setScxmlEvent(const QScxmlEvent &event)
{
    Q_D(QScxmlCppDataModel);
    if (event.name().isEmpty())
        return;

    d->event = event;
}

/*!
 * Holds the current event that is being processed by the
 *        state machine.
 *
 * See also \l {SCXML Specification - 5.10 System Variables} for the description
 * of the \c _event variable.
 *
 * Returns the event currently being processed.
 */
const QScxmlEvent &QScxmlCppDataModel::scxmlEvent() const
{
    Q_D(const QScxmlCppDataModel);
    return d->event;
}

/*!
 * \reimp
 */
QVariant QScxmlCppDataModel::scxmlProperty(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

/*!
 * \reimp
 */
bool QScxmlCppDataModel::hasScxmlProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

/*!
 * \reimp
 */
bool QScxmlCppDataModel::setScxmlProperty(const QString &name, const QVariant &value, const QString &context)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNREACHABLE();
    return false;
}

/*!
 * Returns \c true if the state machine is in the state specified by \a stateName, \c false
 * otherwise.
 */
bool QScxmlCppDataModel::inState(const QString &stateName) const
{
    return stateMachine()->isActive(stateName);
}

QT_END_NAMESPACE
