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

#include <QJsonDocument>
#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>

using namespace Scxml;

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

    void initializeDataFor(QState *s)
    {
        Q_ASSERT(!dataModel.isUndefined());

        foreach (const DataModel::Data &data, q->data()) {
            QJSValue v(QJSValue::UndefinedValue); // See B.2.1, and test456.
            Q_ASSERT(data.context);
            const QString context = QStringLiteral("initializeDataFor with data for %1 defined in state '%2'")
                    .arg(data.id, data.context->objectName());
            bool ok = true;

            if ((dataBinding() == StateTable::EarlyBinding || data.context == table() || data.context == s)
                    && !data.expr.isEmpty()) {
                v = evalJSValue(data.expr, context, &ok);
            }

            qCDebug(scxmlLog) << "setting datamodel property" << data.id << "to" << v.toVariant();
            setProperty(data.id, v, context, &ok);
            Q_ASSERT(dataModel.hasProperty(data.id));
        }
    }


    void setupDataModel()
    {
        Q_ASSERT(engine());
        dataModel = engine()->globalObject();

        qCDebug(scxmlLog) << "initializing the datamodel";
        setupSystemVariables();
        initializeDataFor(table());
    }

    void setupSystemVariables()
    {
        setReadonlyProperty(&dataModel, QStringLiteral("_sessionid"),
                            QStringLiteral("session%1").arg(table()->sessionId()));

        setReadonlyProperty(&dataModel, QStringLiteral("_name"), table()->_name);

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
        _event.setProperty(QStringLiteral("data"), dataValue.isNull() ? QJSValue(QJSValue::UndefinedValue)
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
                return QJSValue(QJSValue::NullValue);
            } else if (event.dataValues().size() == 1) {
                QString data = event.dataValues().first().toString();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError)
                    return engine()->toScriptValue(doc.toVariant());
                else
                    return engine()->toScriptValue(data);
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
}

void EcmaScriptDataModel::initializeDataFor(QState *state)
{
    d->initializeDataFor(state);
}

DataModel::ToStringEvaluator EcmaScriptDataModel::createToStringEvaluator(const QString &expr, const QString &context)
{
    const QString e = expr;
    const QString c = context;
    return [this, e, c](bool *ok) -> QString {
        return d->evalStr(e, c, ok);
    };
}

DataModel::ToBoolEvaluator EcmaScriptDataModel::createToBoolEvaluator(const QString &expr, const QString &context)
{
    const QString e = expr;
    const QString c = context;
    return [this, e, c](bool *ok) -> bool {
        return d->evalBool(e, c, ok);
    };
}

DataModel::ToVariantEvaluator EcmaScriptDataModel::createToVariantEvaluator(const QString &expr, const QString &context)
{
    const QString e = expr;
    const QString c = context;
    return [this, e, c](bool *ok) -> QVariant {
        return d->evalJSValue(e, c, ok).toVariant();
    };
}

DataModel::ToVoidEvaluator EcmaScriptDataModel::createScriptEvaluator(const QString &expr, const QString &context)
{
    const QString e = expr, c = context;
    return [this, e, c](bool *ok) {
        Q_ASSERT(ok);
        d->eval(e, c, ok);
    };
}

DataModel::ToVoidEvaluator EcmaScriptDataModel::createAssignmentEvaluator(const QString &dest,
                                                                        const QString &expr,
                                                                        const QString &context)
{
    const QString t = dest, e = expr, c = context;
    return [this, t, e, c](bool *ok) {
        Q_ASSERT(ok);
        static QByteArray sendid;
        if (hasProperty(t)) {
            QJSValue v = d->evalJSValue(e, c, ok);
            if (*ok)
                d->setProperty(t, v, c, ok);
        } else {
            *ok = false;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2 does not exist").arg(t, c),
                                 sendid);
        }
    };
}

DataModel::ForeachEvaluator EcmaScriptDataModel::createForeachEvaluator(const QString &array,
                                                                        const QString &item,
                                                                        const QString &index,
                                                                        const QString &context)
{
    const QString a = array, it = item, in = index, c = context;
    return [this, a, it, in, c](bool *ok, std::function<bool ()> body) -> bool {
        Q_ASSERT(ok);
        static QByteArray sendid;

        QJSValue jsArray = d->property(a);
        if (!jsArray.isArray()) {
            table()->submitError("error.execution", QStringLiteral("invalid array '%1' in %2").arg(a, c), sendid);
            *ok = false;
            return false;
        }

        if (table()->engine()->evaluate(QStringLiteral("(function(){var %1 = 0})()").arg(it)).isError()) {
            table()->submitError("error.execution", QStringLiteral("invalid item '%1' in %2")
                                 .arg(it, c), sendid);
            *ok = false;
            return false;
        }

        const int length = jsArray.property(QStringLiteral("length")).toInt();
        const bool hasIndex = !in.isEmpty();

        for (int currentIndex = 0; currentIndex < length; ++currentIndex) {
            QJSValue currentItem = jsArray.property(static_cast<quint32>(currentIndex));
            d->setProperty(it, currentItem, c, ok);
            if (!*ok)
                return false;
            if (hasIndex) {
                d->setProperty(in, currentIndex, c, ok);
                if (!*ok)
                    return false;
            }
            if (!body())
                return false;
        }

        return true;
    };
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
