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

#ifndef SCXMLDUMPER_H
#define SCXMLDUMPER_H
#include "scxmlglobals.h"
#include "scxmlstatetable.h"
#include <QXmlStreamWriter>

namespace Scxml {

struct SCXML_EXPORT DumpOptions
{

};

struct SCXML_EXPORT ScxmlDumpOptions : public DumpOptions
{

};

class SCXML_EXPORT ScxmlDumper
{
public:
    ScxmlDumper(QXmlStreamWriter &stream) : s(stream) { }
    void dump(StateTable *table);
    void writeStartElement(const char *name) { s.writeStartElement(QLatin1String(name)); }
    void writeAttribute(const char *name, const char *value)  { s.writeAttribute(QLatin1String(name), QLatin1String(value)); }
    void writeAttribute(const char *name, QByteArray value)  { s.writeAttribute(QLatin1String(name), QString::fromUtf8(value)); }
    void writeAttribute(const char *name, const QString &value)  { s.writeAttribute(QLatin1String(name), value); }
    void writeAttribute(const QString &name, const QString &value)  { s.writeAttribute(name, value); }
    void writeEndElement() { s.writeEndElement(); }
    QXmlStreamWriter &s;
private:
    void scxmlStart();
    bool enterState(QState *state);
    void exitState(QState *state);
    void inAbstractState(QAbstractState *state);

    void dumpTransition(QAbstractTransition *transition);
    void dumpInstruction(const ExecutableContent::Instruction *instruction);

    StateTable *table;
};

}
#endif // SCXMLDUMPER_H
