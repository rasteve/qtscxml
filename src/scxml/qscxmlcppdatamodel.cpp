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

#include "qscxmlcppdatamodel_p.h"
#include "qscxmlstatemachine.h"

QT_USE_NAMESPACE

using namespace QScxmlExecutableContent;

/*!
   \class QScxmlCppDataModel
   \brief C++ data-model for a QScxmlStateMachine
   \since 5.6
   \inmodule QtScxml

   \sa QScxmlStateMachine QScxmlDataModel

   TODO: blah... DIY data-model
   \note TODO: mention that you can inherit from both QScxmlCppDataModel and QObject
   \note put the Q_SCXML_DATAMODEL macro directly after opening the class, so around the same spot
         as the Q_OBJECT macro. The order of these macro uses is not important.

   The Scxml compiler will generate the various evaluateTo methods, and convert expressions and
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
            <send type="qt:signal" event="playbackStarted">
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

   So, you are not limited to call functions. In a script tag you can put zero or more C++ statements,
   and in cond or expr attributes you can use any C++ expression that can be converted to the
   respectively bool or QVariant. And, as the this pointer is also captured, you can call/access
   the data model (the media attribute in the example above). For the full example, see
   mediaplayer-cppdatamodel.
 */

QScxmlCppDataModel::QScxmlCppDataModel()
    : data(new QScxmlCppDataModelPrivate)
{}

QScxmlCppDataModel::~QScxmlCppDataModel()
{
    delete data;
}

void QScxmlCppDataModel::setup(const QVariantMap &initialDataValues)
{
    Q_UNUSED(initialDataValues);
}

void QScxmlCppDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void QScxmlCppDataModel::evaluateInitialization(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool QScxmlCppDataModel::evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNUSED(body);
    Q_UNREACHABLE();
    return false;
}

void QScxmlCppDataModel::setEvent(const QScxmlEvent &event)
{
    data->event = event;
}

const QScxmlEvent &QScxmlCppDataModel::event() const
{
    return data->event;
}

QVariant QScxmlCppDataModel::property(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

bool QScxmlCppDataModel::hasProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

void QScxmlCppDataModel::setProperty(const QString &name, const QVariant &value, const QString &context, bool *ok)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool QScxmlCppDataModel::In(const QString &stateName) const
{
    return stateMachine()->isActive(stateName);
}
