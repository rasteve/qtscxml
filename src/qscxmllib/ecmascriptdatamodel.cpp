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

using namespace Scxml;

class Scxml::EcmaScriptDataModelPrivate
{
public:
    EcmaScriptDataModelPrivate(EcmaScriptDataModel *q)
        : q(q)
    {}

    QString evalStr(const QString &expr, const QString &context, bool *ok)
    {
        Q_ASSERT(ok);
        QJSEngine *e = table()->engine();
        Q_ASSERT(e);

        QJSValue v = e->evaluate(QStringLiteral("(function(){ return (%1).toString(); })()").arg(expr),
                                 QStringLiteral("<expr>"), 1);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2").arg(v.toString(), context),
                                 sendid);
            return QString();
        } else {
            *ok = true;
            return v.toString();
        }
    }

    bool evalBool(const QString &expr, const QString &context, bool *ok)
    {
        Q_ASSERT(ok);
        QJSEngine *e = engine();
        Q_ASSERT(e);

        QJSValue v = e->evaluate(QStringLiteral("(function(){ return !!(%1); })()").arg(expr),
                                 QStringLiteral("<expr>"), 1);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2").arg(v.toString(), context),
                                 sendid);
            return false;
        } else {
            *ok = true;
            return v.toBool();
        }
    }

    QJSValue evalJSValue(const QString &expr, const QString &context, bool *ok)
    {
        Q_ASSERT(engine());

        QString getData = QStringLiteral("(function(){ return (\n%1\n); })()").arg(expr);
        QJSValue v = engine()->evaluate(getData, QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2").arg(v.toString(), context),
                                 sendid);
            return false;
        } else {
            *ok = true;
            return v;
        }
    }

    QJSValue eval(const QString &script, const QString &context, bool *ok)
    {
        QJSValue v = engine()->evaluate(script, QStringLiteral("<expr>"), 0);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            table()->submitError(QByteArray("error.execution"),
                                 QStringLiteral("%1 in %2").arg(v.toString(), context),
                                 sendid);
            return false;
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
            if ((dataBinding() == StateTable::EarlyBinding || !data.context || data.context == s)
                    && !data.expr.isEmpty()) {
                bool ok = true;
                QString context = QStringLiteral("initializeDataFor with data for %1 defined in state '%2'")
                        .arg(data.id, data.context->objectName());
                v = evalJSValue(data.expr, context, &ok);
            }
            qCDebug(scxmlLog) << "setting datamodel property" << data.id << "to" << v.toVariant();
            dataModel.setProperty(data.id, v);
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
        dataModel.setProperty(QStringLiteral("_sessionid"),
                              QStringLiteral("session%1").arg(table()->sessionId()));

        dataModel.setProperty(QStringLiteral("_name"), table()->_name);

        auto scxml = engine()->newObject();
        scxml.setProperty(QStringLiteral("location"), QStringLiteral("TODO")); // TODO
        auto ioProcs = engine()->newObject();
        ioProcs.setProperty(QStringLiteral("scxml"), scxml);
        dataModel.setProperty(QStringLiteral("_ioprocessors"), ioProcs);

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

        dataModel.setProperty(QStringLiteral("_event"), _event);
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
        if (hasProperty(t)) {
            QJSValue v = d->evalJSValue(e, c, ok);
            if (*ok)
                d->dataModel.setProperty(t, v);
        } else {
            *ok = false;
            static QByteArray sendid;
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

        QJSValue jsArray = d->dataModel.property(a);
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
            d->dataModel.setProperty(it, currentItem);
            if (hasIndex)
                d->dataModel.setProperty(in, currentIndex);
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
    return d->dataModel.property(name).toVariant();
}

bool EcmaScriptDataModel::hasProperty(const QString &name) const
{
    return d->dataModel.hasProperty(name);
}

void EcmaScriptDataModel::setStringProperty(const QString &name, const QString &value)
{
    Q_ASSERT(hasProperty(name));
    d->dataModel.setProperty(name, QJSValue(value));
}

QJSEngine *EcmaScriptDataModel::engine() const
{
    return table()->engine();
}
