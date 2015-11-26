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

#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QtScxml/qscxmlexecutablecontent.h>

#include <QVariant>
#include <QVector>

QT_BEGIN_NAMESPACE

class QScxmlEvent;

class QScxmlStateMachine;
class QScxmlTableData;

namespace QScxmlExecutableContent {
#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(push, 4) // 4 == sizeof(qint32)
#endif
struct EvaluatorInfo {
    StringId expr;
    StringId context;
};

struct AssignmentInfo {
    StringId dest;
    StringId expr;
    StringId context;
};

struct ForeachInfo {
    StringId array;
    StringId item;
    StringId index;
    StringId context;
};
#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(pop)
#endif

typedef qint32 EvaluatorId;
enum { NoEvaluator = -1 };
} // QScxmlExecutableContent namespace

class QScxmlDataModelPrivate;
class Q_SCXML_EXPORT QScxmlDataModel
{
    Q_DISABLE_COPY(QScxmlDataModel)

public:
    class ForeachLoopBody
    {
    public:
        virtual ~ForeachLoopBody();
        virtual bool run() = 0;
    };

public:
    QScxmlDataModel();
    virtual ~QScxmlDataModel();

    QScxmlStateMachine *stateMachine() const;

    virtual void setup(const QVariantMap &initialDataValues) = 0;

    virtual QString evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual QVariant evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual void evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual void evaluateAssignment(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual void evaluateInitialization(QScxmlExecutableContent::EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateForeach(QScxmlExecutableContent::EvaluatorId id, bool *ok, ForeachLoopBody *body) = 0;

    virtual void setEvent(const QScxmlEvent &event) = 0;

    virtual QVariant property(const QString &name) const = 0;
    virtual bool hasProperty(const QString &name) const = 0;
    virtual void setProperty(const QString &name, const QVariant &value, const QString &context,
                             bool *ok) = 0;

protected:
    QScxmlTableData *tableData() const;

private:
    friend class QScxmlDataModelPrivate;
    QScxmlDataModelPrivate *d;
};

QT_END_NAMESPACE

#endif // DATAMODEL_H
