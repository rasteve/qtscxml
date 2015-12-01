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

#include "qscxmlecmascriptdatamodel.h"
#include "qscxmlecmascriptplatformproperties_p.h"
#include "qscxmlexecutablecontent_p.h"

#include <QJSEngine>
#include <QJsonDocument>
#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

using namespace QScxmlExecutableContent;

typedef std::function<QString (bool *)> ToStringEvaluator;
typedef std::function<bool (bool *)> ToBoolEvaluator;
typedef std::function<QVariant (bool *)> ToVariantEvaluator;
typedef std::function<void (bool *)> ToVoidEvaluator;
typedef std::function<bool (bool *, std::function<bool ()>)> ForeachEvaluator;

class QScxmlEcmaScriptDataModelPrivate
{
public:
    QScxmlEcmaScriptDataModelPrivate(QScxmlEcmaScriptDataModel *q)
        : q(q)
        , jsEngine(Q_NULLPTR)
    {}

    QString evalStr(const QString &expr, const QString &context, bool *ok)
    {
        QString script = QStringLiteral("(%1).toString()").arg(expr);
        QJSValue v = eval(script, context, ok);
        if (*ok)
            return v.toString();
        else
            return QString();
    }

    bool evalBool(const QString &expr, const QString &context, bool *ok)
    {
        QString script = QStringLiteral("(function(){return !!(%1); })()").arg(expr);
        QJSValue v = eval(script, context, ok);
        if (*ok)
            return v.toBool();
        else
            return false;
    }

    QJSValue evalJSValue(const QString &expr, const QString &context, bool *ok)
    {
        Q_ASSERT(engine());

        QString script = QStringLiteral("(function(){'use strict'; return (\n%1\n); })()").arg(expr);
        return eval(script, context, ok);
    }

    QJSValue eval(const QString &script, const QString &context, bool *ok)
    {
        Q_ASSERT(ok);
        Q_ASSERT(engine());

        // TODO: copy QJSEngine::evaluate and handle the case of v4->catchException() "our way"

        QJSValue v = engine()->evaluate(QStringLiteral("'use strict'; ") + script, QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            *ok = false;
            stateMachine()->submitError(QStringLiteral("error.execution"),
                                        QStringLiteral("%1 in %2").arg(v.toString(), context));
            return QJSValue(QJSValue::UndefinedValue);
        } else {
            *ok = true;
            return v;
        }
    }

    void setupDataModel()
    {
        Q_ASSERT(engine());
        dataModel = engine()->globalObject();

        qCDebug(scxmlLog) << "initializing the datamodel";
        setupSystemVariables();
    }

    void setupSystemVariables()
    {
        setReadonlyProperty(&dataModel, QStringLiteral("_sessionid"),
                            stateMachine()->sessionId());

        setReadonlyProperty(&dataModel, QStringLiteral("_name"), stateMachine()->name());

        auto scxml = engine()->newObject();
        scxml.setProperty(QStringLiteral("location"), QStringLiteral("#_scxml_%1").arg(stateMachine()->sessionId()));
        auto ioProcs = engine()->newObject();
        setReadonlyProperty(&ioProcs, QStringLiteral("scxml"), scxml);
        setReadonlyProperty(&dataModel, QStringLiteral("_ioprocessors"), ioProcs);

        auto platformVars = QScxmlPlatformProperties::create(engine(), stateMachine());
        dataModel.setProperty(QStringLiteral("_x"), platformVars->jsValue());

        dataModel.setProperty(QStringLiteral("In"),
                              engine()->evaluate(QStringLiteral("function(id){return _x.In(id);}")));
    }

    void assignEvent(const QScxmlEvent &event)
    {
        QJSValue _event = engine()->newObject();
        QJSValue dataValue = eventDataAsJSValue(event.data());
        _event.setProperty(QStringLiteral("data"), dataValue.isUndefined() ? QJSValue(QJSValue::UndefinedValue)
                                                                           : dataValue);
        _event.setProperty(QStringLiteral("invokeid"), event.invokeId().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                                  : engine()->toScriptValue(event.invokeId()));
        if (!event.originType().isEmpty())
            _event.setProperty(QStringLiteral("origintype"), engine()->toScriptValue(event.originType()));
        _event.setProperty(QStringLiteral("origin"), event.origin().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                              : engine()->toScriptValue(event.origin()) );
        _event.setProperty(QStringLiteral("sendid"), event.sendId().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                              : engine()->toScriptValue(event.sendId()));
        _event.setProperty(QStringLiteral("type"), engine()->toScriptValue(event.scxmlType()));
        _event.setProperty(QStringLiteral("name"), engine()->toScriptValue(event.name()));
        _event.setProperty(QStringLiteral("raw"), QStringLiteral("unsupported")); // See test178
        if (event.isErrorEvent())
            _event.setProperty(QStringLiteral("errorMessage"), event.errorMessage());

        setReadonlyProperty(&dataModel, QStringLiteral("_event"), _event);
    }

    QJSValue eventDataAsJSValue(const QVariant &eventData) const
    {
        if (!eventData.isValid()) {
            return QJSValue(QJSValue::UndefinedValue);
        }

        if (eventData.canConvert<QVariantMap>()) {
            auto keyValues = eventData.value<QVariantMap>();
            auto data = engine()->newObject();

            for (QVariantMap::const_iterator it = keyValues.begin(), eit = keyValues.end(); it != eit; ++it) {
                data.setProperty(it.key(), engine()->toScriptValue(it.value()));
            }

            return data;
        }

        if (eventData == QVariant(QMetaType::VoidStar, 0)) {
            return QJSValue(QJSValue::NullValue);
        }

        QString data = eventData.toString();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError)
            return engine()->toScriptValue(doc.toVariant());
        else
            return engine()->toScriptValue(data);
    }

    QScxmlStateMachine *stateMachine() const
    { return q->stateMachine(); }

    QJSEngine *engine() const
    {
        if (jsEngine == Q_NULLPTR) {
            jsEngine = new QJSEngine(stateMachine());
        }

        return jsEngine;
    }

    void setEngine(QJSEngine *engine)
    { jsEngine = engine; }

    QString string(StringId id) const
    { return q->tableData()->string(id); }

    QScxmlStateMachine::BindingMethod dataBinding() const
    { return stateMachine()->dataBinding(); }

    bool hasProperty(const QString &name) const
    { return dataModel.hasProperty(name); }

    QJSValue property(const QString &name) const
    { return dataModel.property(name); }

    bool setProperty(const QString &name, const QJSValue &value, const QString &context)
    {
        QString msg;
        switch (setProperty(&dataModel, name, value)) {
        case SetPropertySucceeded:
            return true;
        case SetReadOnlyPropertyFailed:
            msg = QStringLiteral("cannot assign to read-only property %1 in %2");
            break;
        case SetUnknownPropertyFailed:
            msg = QStringLiteral("cannot assign to unknown propety %1 in %2");
            break;
        case SetPropertyFailedForAnotherReason:
            msg = QStringLiteral("assignment to property %1 failed in %2");
            break;
        default:
            Q_UNREACHABLE();
        }

        stateMachine()->submitError(QStringLiteral("error.execution"), msg.arg(name, context));
        return false;
    }

public:
    QStringList initialDataNames;

private: // Uses private API
    static void setReadonlyProperty(QJSValue *object, const QString& name, const QJSValue& value)
    {
        qCDebug(scxmlLog) << "setting read-only property" << name;
        QV4::ExecutionEngine *engine = QJSValuePrivate::engine(object);
        Q_ASSERT(engine);
        QV4::Scope scope(engine);

        QV4::ScopedObject o(scope, QJSValuePrivate::getValue(object));
        if (!o)
            return;

        if (!QJSValuePrivate::checkEngine(engine, value)) {
            qCWarning(scxmlLog, "EcmaScriptDataModel::setReadonlyProperty(%s) failed: cannot set value created in a different engine", name.toUtf8().constData());
            return;
        }

        QV4::ScopedString s(scope, engine->newString(name));
        uint idx = s->asArrayIndex();
        if (idx < UINT_MAX) {
            Q_UNIMPLEMENTED();
            return;
        }

        s->makeIdentifier(scope.engine);
        QV4::ScopedValue v(scope, QJSValuePrivate::convertedToValue(engine, value));
        o->defineReadonlyProperty(s, v);
        if (engine->hasException)
            engine->catchException();
    }

    enum SetPropertyResult {
        SetPropertySucceeded,
        SetReadOnlyPropertyFailed,
        SetUnknownPropertyFailed,
        SetPropertyFailedForAnotherReason,
    };

    static SetPropertyResult setProperty(QJSValue *object, const QString& name, const QJSValue& value)
    {
        QV4::ExecutionEngine *engine = QJSValuePrivate::engine(object);
        Q_ASSERT(engine);
        if (engine->hasException)
            return SetPropertyFailedForAnotherReason;

        QV4::Scope scope(engine);
        QV4::ScopedObject o(scope, QJSValuePrivate::getValue(object));
        if (o == Q_NULLPTR) {
            return SetPropertyFailedForAnotherReason;
        }

        QV4::ScopedString s(scope, engine->newString(name));
        uint idx = s->asArrayIndex();
        if (idx < UINT_MAX) {
            Q_UNIMPLEMENTED();
            return SetPropertyFailedForAnotherReason;
        }

        QV4::PropertyAttributes attrs = o->query(s);
        if (attrs.isWritable() || attrs.isEmpty()) {
            QV4::ScopedValue v(scope, QJSValuePrivate::convertedToValue(engine, value));
            o->insertMember(s, v);
            if (engine->hasException) {
                engine->catchException();
                return SetPropertyFailedForAnotherReason;
            } else {
                return SetPropertySucceeded;
            }
        } else {
            return SetReadOnlyPropertyFailed;
        }
    }

private:
    QScxmlEcmaScriptDataModel *q;
    mutable QJSEngine *jsEngine;
    QJSValue dataModel;
};

/*!
 * \class QScxmlEcmaScriptDataModel
 * \brief The EcmaScript data-model for a QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * This class implements the EcmaScript data-model as described in the SCXML specification. It can
 * be subclassed to do custom initialization.
 *
 * \sa QScxmlStateMachine QScxmlDataModel
 */

/*!
 * \brief Creates a new QScxmlEcmaScriptDataModel.
 */
QScxmlEcmaScriptDataModel::QScxmlEcmaScriptDataModel(QScxmlStateMachine *stateMachine)
    : QScxmlDataModel(stateMachine)
    , d(new QScxmlEcmaScriptDataModelPrivate(this))
{}

/*!
 * \brief Destroys a QScxmlEcmaScriptDataModel
 */
QScxmlEcmaScriptDataModel::~QScxmlEcmaScriptDataModel()
{
    delete d;
}

/*!
 */
bool QScxmlEcmaScriptDataModel::setup(const QVariantMap &initialDataValues)
{
    d->setupDataModel();

    bool ok = true;
    QJSValue undefined(QJSValue::UndefinedValue); // See B.2.1, and test456.
    int count;
    StringId *names = tableData()->dataNames(&count);
    for (int i = 0; i < count; ++i) {
        auto name = d->string(names[i]);
        QJSValue v = undefined;
        QVariantMap::const_iterator it = initialDataValues.find(name);
        if (it != initialDataValues.end()) {
            v = d->engine()->toScriptValue(it.value());
        }
        if (!d->setProperty(name, v, QStringLiteral("<data>"))) {
            ok = false;
        }
    }
    d->initialDataNames = initialDataValues.keys();

    return ok;
}

QString QScxmlEcmaScriptDataModel::evaluateToString(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = tableData()->evaluatorInfo(id);

    return d->evalStr(d->string(info.expr), d->string(info.context), ok);
}

bool QScxmlEcmaScriptDataModel::evaluateToBool(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = tableData()->evaluatorInfo(id);

    return d->evalBool(d->string(info.expr), d->string(info.context), ok);
}

QVariant QScxmlEcmaScriptDataModel::evaluateToVariant(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = tableData()->evaluatorInfo(id);

    return d->evalJSValue(d->string(info.expr), d->string(info.context), ok).toVariant();
}

void QScxmlEcmaScriptDataModel::evaluateToVoid(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = tableData()->evaluatorInfo(id);

    d->eval(d->string(info.expr), d->string(info.context), ok);
}

void QScxmlEcmaScriptDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_ASSERT(ok);

    const AssignmentInfo &info = tableData()->assignmentInfo(id);

    QString dest = d->string(info.dest);

    if (hasProperty(dest)) {
        QJSValue v = d->evalJSValue(d->string(info.expr), d->string(info.context), ok);
        if (*ok)
            *ok = d->setProperty(dest, v, d->string(info.context));
    } else {
        *ok = false;
        stateMachine()->submitError(QStringLiteral("error.execution"),
                                    QStringLiteral("%1 in %2 does not exist").arg(dest, d->string(info.context)));
    }
}

void QScxmlEcmaScriptDataModel::evaluateInitialization(EvaluatorId id, bool *ok)
{
    const AssignmentInfo &info = tableData()->assignmentInfo(id);
    QString dest = d->string(info.dest);
    if (d->initialDataNames.contains(dest)) {
        *ok = true; // silently ignore the <data> tag
        return;
    }

    evaluateAssignment(id, ok);
}

bool QScxmlEcmaScriptDataModel::evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_ASSERT(ok);
    Q_ASSERT(body);
    const ForeachInfo &info = tableData()->foreachInfo(id);

    QJSValue jsArray = d->property(d->string(info.array));
    if (!jsArray.isArray()) {
        stateMachine()->submitError(QStringLiteral("error.execution"), QStringLiteral("invalid array '%1' in %2").arg(d->string(info.array), d->string(info.context)));
        *ok = false;
        return false;
    }

    QString item = d->string(info.item);
    if (engine()->evaluate(QStringLiteral("(function(){var %1 = 0})()").arg(item)).isError()) {
        stateMachine()->submitError(QStringLiteral("error.execution"), QStringLiteral("invalid item '%1' in %2")
                                    .arg(d->string(info.item), d->string(info.context)));
        *ok = false;
        return false;
    }

    const int length = jsArray.property(QStringLiteral("length")).toInt();
    QString idx = d->string(info.index);
    QString context = d->string(info.context);
    const bool hasIndex = !idx.isEmpty();

    for (int currentIndex = 0; currentIndex < length; ++currentIndex) {
        QJSValue currentItem = jsArray.property(static_cast<quint32>(currentIndex));
        *ok = d->setProperty(item, currentItem, context);
        if (!*ok)
            return false;
        if (hasIndex) {
            *ok = d->setProperty(idx, currentIndex, context);
            if (!*ok)
                return false;
        }
        if (!body->run())
            return false;
    }

    return true;
}

/*!
 */
void QScxmlEcmaScriptDataModel::setEvent(const QScxmlEvent &event)
{
    d->assignEvent(event);
}

/*!
 */
QVariant QScxmlEcmaScriptDataModel::property(const QString &name) const
{
    return d->property(name).toVariant();
}

/*!
 */
bool QScxmlEcmaScriptDataModel::hasProperty(const QString &name) const
{
    return d->hasProperty(name);
}

/*!
 */
bool QScxmlEcmaScriptDataModel::setProperty(const QString &name, const QVariant &value, const QString &context)
{
    Q_ASSERT(hasProperty(name));
    QJSValue v = d->engine()->toScriptValue(value);
    return d->setProperty(name, v, context);
}

/*!
 * \return The JavaScript engine used by this data-model.
 */
QJSEngine *QScxmlEcmaScriptDataModel::engine() const
{
    return d->engine();
}

/*!
 * \brief Sets the JavaScript engine used by this data-model to \a engine .
 */
void QScxmlEcmaScriptDataModel::setEngine(QJSEngine *engine)
{
    d->setEngine(engine);
}

QT_END_NAMESPACE
