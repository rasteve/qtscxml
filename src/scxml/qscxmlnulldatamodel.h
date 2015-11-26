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

#ifndef NULLDATAMODEL_H
#define NULLDATAMODEL_H

#include <QtScxml/qscxmldatamodel.h>

QT_BEGIN_NAMESPACE

class Q_SCXML_EXPORT QScxmlNullDataModel: public QScxmlDataModel
{
public:
    QScxmlNullDataModel();
    ~QScxmlNullDataModel();

    void setup(const QVariantMap &initialDataValues) Q_DECL_OVERRIDE;

    QString evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    bool evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    QVariant evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void evaluateAssignment(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void evaluateInitialization(QScxmlExecutableContent::EvaluatorId id, bool *ok) Q_DECL_OVERRIDE Q_DECL_FINAL;
    bool evaluateForeach(QScxmlExecutableContent::EvaluatorId id, bool *ok, ForeachLoopBody *body) Q_DECL_OVERRIDE Q_DECL_FINAL;

    void setEvent(const QScxmlEvent &event) Q_DECL_OVERRIDE;

    QVariant property(const QString &name) const Q_DECL_OVERRIDE;
    bool hasProperty(const QString &name) const Q_DECL_OVERRIDE;
    void setProperty(const QString &name, const QVariant &value, const QString &context, bool *ok) Q_DECL_OVERRIDE;

private:
    class Data;
    friend Data;
    Data *d;
};

QT_END_NAMESPACE

#endif // NULLDATAMODEL_H
