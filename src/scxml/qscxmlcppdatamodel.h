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

#ifndef CPPDATAMODEL_H
#define CPPDATAMODEL_H

#include <QtScxml/qscxmldatamodel.h>

#define Q_SCXML_DATAMODEL \
    public: \
        QString evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE; \
        bool evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE; \
        QVariant evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE; \
        void evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;

QT_BEGIN_NAMESPACE

class QScxmlCppDataModelPrivate;
class Q_SCXML_EXPORT QScxmlCppDataModel: public QScxmlDataModel
{
public:
    QScxmlCppDataModel();
    ~QScxmlCppDataModel();

    void setup(const QVariantMap &initialDataValues) Q_DECL_OVERRIDE;

    void evaluateAssignment(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    void evaluateInitialization(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE;
    bool evaluateForeach(QScxmlExecutableContent::EvaluatorId id, bool *ok, ForeachLoopBody *body) Q_DECL_OVERRIDE;

    void setEvent(const QScxmlEvent &event) Q_DECL_OVERRIDE;
    const QScxmlEvent &event() const;

    QVariant property(const QString &name) const Q_DECL_OVERRIDE;
    bool hasProperty(const QString &name) const Q_DECL_OVERRIDE;
    void setProperty(const QString &name, const QVariant &value, const QString &context,
                     bool *ok) Q_DECL_OVERRIDE;

    bool In(const QString &stateName) const;

private:
    QScxmlCppDataModelPrivate *data;
};

QT_END_NAMESPACE

#endif // CPPDATAMODEL_H
