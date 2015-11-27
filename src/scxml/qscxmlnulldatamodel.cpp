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

#include "qscxmlnulldatamodel.h"
#include "qscxmlevent.h"
#include "qscxmlstatemachine.h"
#include "qscxmltabledata.h"

class QScxmlNullDataModel::Data
{
    struct ResolvedEvaluatorInfo {
        bool error;
        QString str;

        ResolvedEvaluatorInfo()
            : error(false)
        {}
    };

public:
    Data(QScxmlNullDataModel *dataModel)
        : q(dataModel)
    {}

    bool evalBool(QScxmlExecutableContent::EvaluatorId id, bool *ok)
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

    ResolvedEvaluatorInfo prepare(QScxmlExecutableContent::EvaluatorId id)
    {
        auto td = q->tableData();
        const QScxmlExecutableContent::EvaluatorInfo &info = td->evaluatorInfo(id);
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
    QScxmlNullDataModel *q;
    typedef QHash<QScxmlExecutableContent::EvaluatorId, ResolvedEvaluatorInfo> Resolved;
    Resolved resolved;
};

/*!
 * \class QScxmlNullDataModel
 * \brief The "null" data-model for a QScxmlStateMachine
 * \since 5.6
 * \inmodule QtScxml
 *
 * This class implements the "null" data-model as described in the SCXML specification.
 *
 * \sa QScxmlStateMachine QScxmlDataModel
 */

QScxmlNullDataModel::QScxmlNullDataModel()
    : d(new Data(this))
{}

QScxmlNullDataModel::~QScxmlNullDataModel()
{
    delete d;
}

bool QScxmlNullDataModel::setup(const QVariantMap &initialDataValues)
{
    Q_UNUSED(initialDataValues);

    return true;
}

QString QScxmlNullDataModel::evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    // We do implement this, because <log> is allowed in the Null data model,
    // and <log> has an expr attribute that needs "evaluation" for it to generate the log message.
    *ok = true;
    auto td = tableData();
    const QScxmlExecutableContent::EvaluatorInfo &info = td->evaluatorInfo(id);
    return td->string(info.expr);
}

bool QScxmlNullDataModel::evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    return d->evalBool(id, ok);
}

QVariant QScxmlNullDataModel::evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
    return QVariant();
}

void QScxmlNullDataModel::evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void QScxmlNullDataModel::evaluateAssignment(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

void QScxmlNullDataModel::evaluateInitialization(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}

bool QScxmlNullDataModel::evaluateForeach(QScxmlExecutableContent::EvaluatorId id, bool *ok, ForeachLoopBody *body)
{
    Q_UNUSED(id);
    Q_UNUSED(ok);
    Q_UNUSED(body);
    Q_UNREACHABLE();
    return false;
}

void QScxmlNullDataModel::setEvent(const QScxmlEvent &event)
{
    Q_UNUSED(event);
}

QVariant QScxmlNullDataModel::property(const QString &name) const
{
    Q_UNUSED(name);
    return QVariant();
}

bool QScxmlNullDataModel::hasProperty(const QString &name) const
{
    Q_UNUSED(name);
    return false;
}

void QScxmlNullDataModel::setProperty(const QString &name, const QVariant &value, const QString &context, bool *ok)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(context);
    Q_UNUSED(ok);
    Q_UNREACHABLE();
}
