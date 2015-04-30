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
    // TODO: move implementation to .cpp file.
    class SCXML_EXPORT PlatformProperties: public QObject
    {
        Q_OBJECT

        PlatformProperties &operator=(const PlatformProperties &) = delete;

        PlatformProperties(QObject *parent)
            : QObject(parent)
            , m_table(0)
        {}

        Q_PROPERTY(QString marks READ marks CONSTANT)

    public:
        static PlatformProperties *create(QJSEngine *engine, StateTable *table)
        {
            PlatformProperties *pp = new PlatformProperties(engine);
            pp->m_table = table;
            pp->m_jsValue = engine->newQObject(pp);
            return pp;
        }

        QJSEngine *engine() const { return qobject_cast<QJSEngine *>(parent()); }
        StateTable *table() const { return m_table; }
        QJSValue jsValue() const { return m_jsValue; }

        QString marks() const
        {
            return QStringLiteral("the spot");
        }

        Q_INVOKABLE bool In(const QString &stateName)
        {
            foreach (QAbstractState *s, table()->configuration()) {
                if (s->objectName() == stateName)
                    return true;
            }

            return false;
        }

    private:
        StateTable *m_table;
        QJSValue m_jsValue;
    };

public:
    EcmaScriptDataModel(StateTable *table);
    ~EcmaScriptDataModel() Q_DECL_OVERRIDE;

    void setup() Q_DECL_OVERRIDE;
    void initializeDataFor(QState *state) Q_DECL_OVERRIDE;
    EvaluatorString createEvaluatorString(const QString &expr, const QString &context) Q_DECL_OVERRIDE;
    EvaluatorBool createEvaluatorBool(const QString &expr, const QString &context) Q_DECL_OVERRIDE;
    StringPropertySetter createStringPropertySetter(const QString &propertyName) Q_DECL_OVERRIDE;

    void assignEvent(const ScxmlEvent &event) Q_DECL_OVERRIDE;
    QVariant propertyValue(const QString &name) const Q_DECL_OVERRIDE;

    QJSEngine *engine() const;

private:
    EcmaScriptDataModelPrivate *d;
};

} // Scxml namespace

#endif // ECMASCRIPTDATAMODEL_H
