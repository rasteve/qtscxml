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

#include "ecmascriptdatamodel.h"
#include "ecmascriptplatformproperties.h"
#include "executablecontent_p.h"

#include <QJsonDocument>
#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>

using namespace Scxml;

typedef std::function<QString (bool *)> ToStringEvaluator;
typedef std::function<bool (bool *)> ToBoolEvaluator;
typedef std::function<QVariant (bool *)> ToVariantEvaluator;
typedef std::function<void (bool *)> ToVoidEvaluator;
typedef std::function<bool (bool *, std::function<bool ()>)> ForeachEvaluator;

class Scxml::EcmaScriptDataModelPrivate
{
public:
    EcmaScriptDataModelPrivate(EcmaScriptDataModel *q)
        : q(q)
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

        // FIXME: copy QJSEngine::evaluate and handle the case of v4->catchException() "our way"

        QJSValue v = engine()->evaluate(QStringLiteral("'use strict'; ") + script, QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2").arg(v.toString(), context),
                                 sendid);
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
                            QStringLiteral("session%1").arg(table()->sessionId()));

        setReadonlyProperty(&dataModel, QStringLiteral("_name"), table()->name());

        auto scxml = engine()->newObject();
        scxml.setProperty(QStringLiteral("location"), QStringLiteral("#_scxml_%1").arg(table()->sessionId()));
        auto ioProcs = engine()->newObject();
        setReadonlyProperty(&ioProcs, QStringLiteral("scxml"), scxml);
        setReadonlyProperty(&dataModel, QStringLiteral("_ioprocessors"), ioProcs);

        auto platformVars = PlatformProperties::create(engine(), table());
        dataModel.setProperty(QStringLiteral("_x"), platformVars->jsValue());

        dataModel.setProperty(QStringLiteral("In"),
                              engine()->evaluate(QStringLiteral("function(id){return _x.In(id);}")));
    }

    void assignEvent(const ScxmlEvent &event)
    {
        QJSValue _event = engine()->newObject();
        QJSValue dataValue = eventDataAsJSValue(event);
        _event.setProperty(QStringLiteral("data"), dataValue.isUndefined() ? QJSValue(QJSValue::UndefinedValue)
                                                                           : dataValue);
        _event.setProperty(QStringLiteral("invokeid"), event.invokeid().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                                  : engine()->toScriptValue(QString::fromUtf8(event.invokeid())));
        if (!event.origintype().isEmpty())
            _event.setProperty(QStringLiteral("origintype"), engine()->toScriptValue(event.origintype()));
        _event.setProperty(QStringLiteral("origin"), event.origin().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                              : engine()->toScriptValue(event.origin()) );
        _event.setProperty(QStringLiteral("sendid"), event.sendid().isEmpty() ? QJSValue(QJSValue::UndefinedValue)
                                                                              : engine()->toScriptValue(QString::fromUtf8(event.sendid())));
        _event.setProperty(QStringLiteral("type"), engine()->toScriptValue(event.scxmlType()));
        _event.setProperty(QStringLiteral("name"), engine()->toScriptValue(QString::fromUtf8(event.name())));
        _event.setProperty(QStringLiteral("raw"), QStringLiteral("unsupported")); // See test178
        // TODO: document this

        setReadonlyProperty(&dataModel, QStringLiteral("_event"), _event);
    }

    QJSValue eventDataAsJSValue(const ScxmlEvent &event) const
    {
        if (event.dataNames().isEmpty()) {
            if (event.dataValues().size() == 0) {
                return QJSValue(QJSValue::UndefinedValue);
            } else if (event.dataValues().size() == 1) {
                auto dataValue = event.dataValues().first();
                if (dataValue == QVariant(QMetaType::VoidStar, 0)) {
                    return QJSValue(QJSValue::NullValue);
                } else {
                    QString data = dataValue.toString();
                    QJsonParseError err;
                    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &err);
                    if (err.error == QJsonParseError::NoError)
                        return engine()->toScriptValue(doc.toVariant());
                    else
                        return engine()->toScriptValue(data);
                }
            } else {
                Q_UNREACHABLE();
                return QJSValue(QJSValue::UndefinedValue);
            }
        } else {
            auto data = engine()->newObject();

            for (int i = 0, ei = std::min(event.dataNames().size(), event.dataValues().size()); i != ei; ++i) {
                data.setProperty(event.dataNames().at(i), engine()->toScriptValue(event.dataValues().at(i)));
            }

            return data;
        }
    }

    StateTable *table() const
    { return q->table(); }

    QJSEngine *engine() const
    { return q->engine(); }

    QString string(ExecutableContent::StringId id) const
    { return table()->tableData()->string(id); }

    StateTable::BindingMethod dataBinding() const
    { return table()->dataBinding(); }

    bool hasProperty(const QString &name) const
    { return dataModel.hasProperty(name); }

    QJSValue property(const QString &name) const
    { return dataModel.property(name); }

    void setProperty(const QString &name, const QJSValue &value, const QString &context, bool *ok)
    {
        Q_ASSERT(ok);

        QString msg;
        switch (setProperty(&dataModel, name, value)) {
        case SetPropertySucceeded:
            *ok = true;
            return;
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

        *ok = false;
        static QByteArray sendid;
        table()->submitError(QByteArray("error.execution"), msg.arg(name, context), sendid);
    }

private: // Uses private API
    static void setReadonlyProperty(QJSValue *object, const QString& name, const QJSValue& value)
    {
        qDebug()<<"setting read-only property"<<name;
        QV4::ExecutionEngine *engine = QJSValuePrivate::engine(object);
        Q_ASSERT(engine);
        QV4::Scope scope(engine);

        QV4::ScopedObject o(scope, QJSValuePrivate::getValue(object));
        if (!o)
            return;

        if (!QJSValuePrivate::checkEngine(engine, value)) {
            qWarning("EcmaScriptDataModel::setReadonlyProperty(%s) failed: cannot set value created in a different engine", name.toUtf8().constData());
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
        if (o == nullptr) {
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
    EcmaScriptDataModel *q;
    QJSValue dataModel;
};

EcmaScriptDataModel::EcmaScriptDataModel(StateTable *table)
    : DataModel(table)
    , d(new EcmaScriptDataModelPrivate(this))
{}

EcmaScriptDataModel::~EcmaScriptDataModel()
{
    delete d;
}

void EcmaScriptDataModel::setup()
{
    d->setupDataModel();

    bool ok;
    QJSValue v(QJSValue::UndefinedValue); // See B.2.1, and test456.
    int count;
    ExecutableContent::StringId *names = table()->tableData()->dataNames(&count);
    for (int i = 0; i < count; ++i)
        d->setProperty(d->string(names[i]), v, QStringLiteral("<data>"), &ok);

}

QString EcmaScriptDataModel::evaluateToString(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = table()->tableData()->evaluatorInfo(id);

    return d->evalStr(d->string(info.expr), d->string(info.context), ok);
}

bool EcmaScriptDataModel::evaluateToBool(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = table()->tableData()->evaluatorInfo(id);

    return d->evalBool(d->string(info.expr), d->string(info.context), ok);
}

QVariant EcmaScriptDataModel::evaluateToVariant(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = table()->tableData()->evaluatorInfo(id);

    return d->evalJSValue(d->string(info.expr), d->string(info.context), ok).toVariant();
}

void EcmaScriptDataModel::evaluateToVoid(EvaluatorId id, bool *ok)
{
    const EvaluatorInfo &info = table()->tableData()->evaluatorInfo(id);

    d->eval(d->string(info.expr), d->string(info.context), ok);
}

void EcmaScriptDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_ASSERT(ok);

    const AssignmentInfo &info = table()->tableData()->assignmentInfo(id);
    static QByteArray sendid;

    QString dest = d->string(info.dest);

    if (hasProperty(dest)) {
        QJSValue v = d->evalJSValue(d->string(info.expr), d->string(info.context), ok);
        if (*ok)
            d->setProperty(dest, v, d->string(info.context), ok);
    } else {
        *ok = false;
        table()->submitError(QByteArray("error.execution"),
                             QStringLiteral("%1 in %2 does not exist").arg(dest, d->string(info.context)),
                             sendid);
    }
}

bool EcmaScriptDataModel::evaluateForeach(EvaluatorId id, bool *ok, std::function<bool ()> body)
{
    Q_ASSERT(ok);
    static QByteArray sendid;
    const ForeachInfo &info = table()->tableData()->foreachInfo(id);

    QJSValue jsArray = d->property(d->string(info.array));
    if (!jsArray.isArray()) {
        table()->submitError("error.execution", QStringLiteral("invalid array '%1' in %2").arg(d->string(info.array), d->string(info.context)), sendid);
        *ok = false;
        return false;
    }

    QString item = d->string(info.item);
    if (table()->engine()->evaluate(QStringLiteral("(function(){var %1 = 0})()").arg(item)).isError()) {
        table()->submitError("error.execution", QStringLiteral("invalid item '%1' in %2")
                             .arg(d->string(info.item), d->string(info.context)), sendid);
        *ok = false;
        return false;
    }

    const int length = jsArray.property(QStringLiteral("length")).toInt();
    QString idx = d->string(info.index);
    QString context = d->string(info.context);
    const bool hasIndex = !idx.isEmpty();

    for (int currentIndex = 0; currentIndex < length; ++currentIndex) {
        QJSValue currentItem = jsArray.property(static_cast<quint32>(currentIndex));
        d->setProperty(item, currentItem, context, ok);
        if (!*ok)
            return false;
        if (hasIndex) {
            d->setProperty(idx, currentIndex, context, ok);
            if (!*ok)
                return false;
        }
        if (!body())
            return false;
    }

    return true;
}

void EcmaScriptDataModel::setEvent(const ScxmlEvent &event)
{
    d->assignEvent(event);
}

QVariant EcmaScriptDataModel::property(const QString &name) const
{
    return d->property(name).toVariant();
}

bool EcmaScriptDataModel::hasProperty(const QString &name) const
{
    return d->hasProperty(name);
}

void EcmaScriptDataModel::setStringProperty(const QString &name, const QString &value, const QString &context, bool *ok)
{
    Q_ASSERT(hasProperty(name));
    d->setProperty(name, QJSValue(value), context, ok);
}

QJSEngine *EcmaScriptDataModel::engine() const
{
    return table()->engine();
}
