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

#include "qscxmlecmascriptplatformproperties_p.h"
#include "qscxmlstatemachine.h"

QT_BEGIN_NAMESPACE
class QScxmlPlatformProperties::Data
{
public:
    Data()
        : m_table(Q_NULLPTR)
    {}

    QScxmlStateMachine *m_table;
    QJSValue m_jsValue;
};
QT_END_NAMESPACE

QScxmlPlatformProperties::QScxmlPlatformProperties(QObject *parent)
    : QObject(parent)
    , data(new Data)
{}

QScxmlPlatformProperties *QScxmlPlatformProperties::create(QJSEngine *engine, QScxmlStateMachine *table)
{
    QScxmlPlatformProperties *pp = new QScxmlPlatformProperties(engine);
    pp->data->m_table = table;
    pp->data->m_jsValue = engine->newQObject(pp);
    return pp;
}

QScxmlPlatformProperties::~QScxmlPlatformProperties()
{
    delete data;
}

QJSEngine *QScxmlPlatformProperties::engine() const
{
    return qobject_cast<QJSEngine *>(parent());
}

QScxmlStateMachine *QScxmlPlatformProperties::stateMachine() const
{
    return data->m_table;
}

QJSValue QScxmlPlatformProperties::jsValue() const
{
    return data->m_jsValue;
}

/// _x.marks() == "the spot"
QString QScxmlPlatformProperties::marks() const
{
    return QStringLiteral("the spot");
}

bool QScxmlPlatformProperties::In(const QString &stateName)
{
    return stateMachine()->isActive(stateName);
}
