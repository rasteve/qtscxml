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

#include "executablecontent.h"

#include <QVariant>
#include <QVector>

namespace Scxml {

class ScxmlEvent;
class StateMachine;

#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(push, 4) // 4 == sizeof(qint32)
#endif
struct EvaluatorInfo {
    ExecutableContent::StringId expr;
    ExecutableContent::StringId context;
};

struct AssignmentInfo {
    ExecutableContent::StringId dest;
    ExecutableContent::StringId expr;
    ExecutableContent::StringId context;
};

struct ForeachInfo {
    ExecutableContent::StringId array;
    ExecutableContent::StringId item;
    ExecutableContent::StringId index;
    ExecutableContent::StringId context;
};
#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(pop)
#endif

typedef qint32 EvaluatorId;
enum { NoEvaluator = -1 };

class NullDataModel;
class EcmaScriptDataModel;
class SCXML_EXPORT DataModel
{
    Q_DISABLE_COPY(DataModel)

public:
    class ForeachLoopBody
    {
    public:
        virtual ~ForeachLoopBody();
        virtual bool run() = 0;
    };

public:
    DataModel();
    virtual ~DataModel();

    StateMachine *stateMachine() const;
    void setTable(StateMachine *stateMachine);

    virtual void setup() = 0;

    virtual QString evaluateToString(EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateToBool(EvaluatorId id, bool *ok) = 0;
    virtual QVariant evaluateToVariant(EvaluatorId id, bool *ok) = 0;
    virtual void evaluateToVoid(EvaluatorId id, bool *ok) = 0;
    virtual void evaluateAssignment(EvaluatorId id, bool *ok) = 0;
    virtual bool evaluateForeach(EvaluatorId id, bool *ok, ForeachLoopBody *body) = 0;

    virtual void setEvent(const ScxmlEvent &event) = 0;

    virtual QVariant property(const QString &name) const = 0;
    virtual bool hasProperty(const QString &name) const = 0;
    virtual void setStringProperty(const QString &name, const QString &value, const QString &context,
                                   bool *ok) = 0;

    virtual NullDataModel *asNullDataModel();
    virtual EcmaScriptDataModel *asEcmaScriptDataModel();

private:
    class Data;
    Data *d;
};

} // namespace Scxml

#endif // DATAMODEL_H
