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

#ifndef EXECUTABLECONTENT_H
#define EXECUTABLECONTENT_H

#include <QScxml/scxmlglobals.h>

#include <QByteArray>
#include <QString>
#include <QVariant>

QT_BEGIN_NAMESPACE

namespace Scxml {

class StateMachine;

namespace ExecutableContent {

typedef int ContainerId;
enum { NoInstruction = -1 };
typedef qint32 StringId;
typedef QVector<StringId> StringIds;
enum { NoString = -1 };
typedef qint32 ByteArrayId;
typedef qint32 *Instructions;

class SCXML_EXPORT ExecutionEngine
{
public:
    ExecutionEngine(StateMachine *table);
    ~ExecutionEngine();

    bool execute(ContainerId ip, const QVariant &extraData = QVariant());

private:
    class Data;
    Data *data;
};

} // ExecutableContent namespace
} // namespace Scxml

QT_END_NAMESPACE

#endif // EXECUTABLECONTENT_H
