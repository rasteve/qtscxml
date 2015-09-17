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

#ifndef ECMASCRIPTDATAMODEL_H
#define ECMASCRIPTDATAMODEL_H

#include "datamodel.h"

QT_BEGIN_NAMESPACE
class QJSEngine;

class QScxmlEcmaScriptDataModelPrivate;
class Q_SCXML_EXPORT QScxmlEcmaScriptDataModel: public QScxmlDataModel
{
public:
    QScxmlEcmaScriptDataModel();
    ~QScxmlEcmaScriptDataModel() Q_DECL_OVERRIDE;

    void setup(const QVariantMap &initialDataValues) Q_DECL_OVERRIDE;

    QString evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    bool evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    QVariant evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    void evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    void evaluateAssignment(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    void evaluateInitialization(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    bool evaluateForeach(QScxmlExecutableContent::EvaluatorId id, bool *ok, ForeachLoopBody *body) Q_DECL_OVERRIDE;

    void setEvent(const QScxmlEvent &event) Q_DECL_OVERRIDE;

    QVariant property(const QString &name) const Q_DECL_OVERRIDE;
    bool hasProperty(const QString &name) const Q_DECL_OVERRIDE;
    void setProperty(const QString &name, const QVariant &value, const QString &context, bool *ok) Q_DECL_OVERRIDE;

    QJSEngine *engine() const;
    void setEngine(QJSEngine *engine);

private:
    QScxmlEcmaScriptDataModelPrivate *d;
};

QT_END_NAMESPACE

#endif // ECMASCRIPTDATAMODEL_H
