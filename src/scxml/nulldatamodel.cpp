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

#include "nulldatamodel.h"
#include "scxmlevent.h"

#include "scxmlstatemachine.h"

using namespace Scxml;

class NullDataModel::Data
{
    struct ResolvedEvaluatorInfo {
        bool error = false;
        QString str;
    };

public:
    Data(NullDataModel *dataModel)
        : q(dataModel)
    {}

    bool evalBool(EvaluatorId id, bool *ok)
    {
        Q_ASSERT(ok);

        ResolvedEvaluatorInfo info;
        Resolved::const_iterator it = resolved.find(id);
        if (it == resolved.end()) {
            info = prepare(id);
        } else {
            info = it.value();
        }

        if (info.error) {
            *ok = false;
            static QByteArray sendid;
            q->stateMachine()->submitError(QByteArray("error.execution"), info.str, sendid);
            return false;
        }

        return q->stateMachine()->isActive(info.str);
    }

    ResolvedEvaluatorInfo prepare(EvaluatorId id)
    {
        auto td = q->stateMachine()->tableData();
        const EvaluatorInfo &info = td->evaluatorInfo(id);
        QString expr = td->string(info.expr);
        for (int i = 0; i < expr.size(); ) {
            QChar ch = expr.at(i);
            if (ch.isSpace()) {
                expr.remove(i, 1);
            } else {
                ++i;
            }
        }

        ResolvedEvaluatorInfo resolved;
        if (expr.startsWith(QStringLiteral("In(")) && expr.endsWith(QLatin1Char(')'))) {
            resolved.error = false;
            resolved.str =  expr.mid(3, expr.length() - 4);
        } else {
            resolved.error = true;
            resolved.str =  QStringLiteral("%1 in %2").arg(expr, td->string(info.context));
        }
        return qMove(resolved);
    }

private:
    NullDataModel *q;
    typedef QHash<EvaluatorId, ResolvedEvaluatorInfo> Resolved;
    Resolved resolved;
};

NullDataModel::NullDataModel()
    : d(new Data(this))
{}

NullDataModel::~NullDataModel()
{
    delete d;
}

void NullDataModel::setup()
{
}

QString NullDataModel::evaluateToString(EvaluatorId id, bool *ok)
{
    // We do implement this, because <log> is allowed in the Null data model,
    // and <log> has an expr attribute that needs "evaluation" for it to generate the log message.
    *ok = true;
    auto td = stateMachine()->tableData();
    const EvaluatorInfo &info = td->evaluatorInfo(id);
    return td->string(info.expr);
}

bool NullDataModel::evaluateToBool(EvaluatorId id, bool *ok)
{
    return d->evalBool(id, ok);
}

QVariant NullDataModel::evaluateToVariant(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
    return QVariant();
}

void NullDataModel::evaluateToVoid(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void NullDataModel::evaluateAssignment(EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool NullDataModel::evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNUSED(body);
    Q_UNREACHABLE();
    return false;
}

void NullDataModel::setEvent(const ScxmlEvent &event)
{
    Q_UNUSED(event);
}

QVariant NullDataModel::property(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

bool NullDataModel::hasProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

void NullDataModel::setStringProperty(const QString &name, const QString &value, const QString &context, bool *ok)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

NullDataModel *NullDataModel::asNullDataModel()
{
    return this;
}
