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

#include "ecmascriptplatformproperties_p.h"
#include "scxmlstatemachine.h"

QT_BEGIN_NAMESPACE
namespace Scxml {
class PlatformProperties::Data
{
public:
    Data()
        : m_table(Q_NULLPTR)
    {}

    QScxmlStateMachine *m_table;
    QJSValue m_jsValue;
};
} // Scxml namespace
QT_END_NAMESPACE

using namespace Scxml;

PlatformProperties::PlatformProperties(QObject *parent)
    : QObject(parent)
    , data(new Data)
{}

PlatformProperties *PlatformProperties::create(QJSEngine *engine, QScxmlStateMachine *table)
{
    PlatformProperties *pp = new PlatformProperties(engine);
    pp->data->m_table = table;
    pp->data->m_jsValue = engine->newQObject(pp);
    return pp;
}

PlatformProperties::~PlatformProperties()
{
    delete data;
}

QJSEngine *PlatformProperties::engine() const
{
    return qobject_cast<QJSEngine *>(parent());
}

QScxmlStateMachine *PlatformProperties::stateMachine() const
{
    return data->m_table;
}

QJSValue PlatformProperties::jsValue() const
{
    return data->m_jsValue;
}

/// _x.marks() == "the spot"
QString PlatformProperties::marks() const
{
    return QStringLiteral("the spot");
}

bool PlatformProperties::In(const QString &stateName)
{
    return stateMachine()->isActive(stateName);
}
