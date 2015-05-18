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

#include "scxmlstatetable.h"

namespace Scxml {

class EcmaScriptDataModelPrivate;
class SCXML_EXPORT EcmaScriptDataModel: public DataModel
{
public:
    EcmaScriptDataModel(StateTable *table);
    ~EcmaScriptDataModel() Q_DECL_OVERRIDE;

    void setup() Q_DECL_OVERRIDE;
    void initializeDataFor(QState *state) Q_DECL_OVERRIDE;
    ToStringEvaluator createToStringEvaluator(const QString &expr, const QString &context) Q_DECL_OVERRIDE;
    ToBoolEvaluator createToBoolEvaluator(const QString &expr, const QString &context) Q_DECL_OVERRIDE;
    ToVariantEvaluator createToVariantEvaluator(const QString &expr, const QString &context) Q_DECL_OVERRIDE;
    ToVoidEvaluator createScriptEvaluator(const QString &script, const QString &context) Q_DECL_OVERRIDE;
    ToVoidEvaluator createAssignmentEvaluator(const QString &dest, const QString &expr,
                                              const QString &context) Q_DECL_OVERRIDE;
    ForeachEvaluator createForeachEvaluator(const QString &array, const QString &item,
                                            const QString &index, const QString &context) Q_DECL_OVERRIDE;

    void setEvent(const ScxmlEvent &event) Q_DECL_OVERRIDE;

    QVariant property(const QString &name) const Q_DECL_OVERRIDE;
    bool hasProperty(const QString &name) const Q_DECL_OVERRIDE;
    void setStringProperty(const QString &name, const QString &value, const QString &context, bool *ok) Q_DECL_OVERRIDE;

    QJSEngine *engine() const;

private:
    EcmaScriptDataModelPrivate *d;
};

} // Scxml namespace

#endif // ECMASCRIPTDATAMODEL_H
