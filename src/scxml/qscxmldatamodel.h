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

#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QtScxml/qscxmlexecutablecontent.h>

#include <QVariant>
#include <QVector>

QT_BEGIN_NAMESPACE

class QScxmlEvent;

class QScxmlStateMachine;

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

class QScxmlNullDataModel;
class QScxmlEcmaScriptDataModel;
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
    void setTable(QScxmlStateMachine *stateMachine);

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

private:
    QScxmlDataModelPrivate *d;
};

QT_END_NAMESPACE

#endif // DATAMODEL_H
