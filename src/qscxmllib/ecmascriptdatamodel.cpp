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

using namespace Scxml;

class Scxml::EcmaScriptDataModelPrivate
{
    EcmaScriptDataModel *q;
public:
    EcmaScriptDataModelPrivate(EcmaScriptDataModel *q)
        : q(q)
    {}

    QString evalStr(const QString &expr, const QString &context, bool *ok)
    {
        Q_ASSERT(ok);
        QJSEngine *e = q->table()->engine();
        Q_ASSERT(e);

        QJSValue v = e->evaluate(QStringLiteral("(function(){ return (%1).toString(); })()").arg(expr),
                                 QStringLiteral("<expr>"), 1);
        if (v.isError()) {
            *ok = false;
            static QByteArray sendid;
            q->table()->submitError(QByteArray("error.execution"),
                                    QStringLiteral("%1 in %2").arg(v.toString(), context),
                                    sendid);
            return QString();
        } else {
            *ok = true;
            return v.toString();
        }
    }
};

EcmaScriptDataModel::EcmaScriptDataModel(StateTable *table)
    : DataModel(table)
    , d(new EcmaScriptDataModelPrivate(this))
{}

EcmaScriptDataModel::~EcmaScriptDataModel()
{
    delete d;
}

DataModel::EvaluatorString EcmaScriptDataModel::createEvaluator(const QString &expr, const QString &context)
{
    const QString e = expr;
    const QString c = context;
    return [this, e, c](bool *ok) -> QString {
        return d->evalStr(e, c, ok);
    };
}
