/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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

#ifndef CPPDUMPER_H
#define CPPDUMPER_H
#include "scxmldumper.h"
#include <QTextStream>

namespace Scxml {

struct SCXML_EXPORT CppDumpOptions : public DumpOptions
{
    QByteArray basename;
    QByteArray namespaceName;
};

class CppDumper
{
public:
    CppDumper(QTextStream &stream, const CppDumpOptions &options) : s(stream), options(options) { }
    void dump(StateTable *table);
    QTextStream &s;
private:
    void dumpDeclareStates();
    void dumpDeclareSignalsForEvents();
    void dumpExecutableContent();
    void dumpInit();

    static QByteArray b(const char *str) { return QByteArray(str); }
    static QLatin1String l (const char *str) { return QLatin1String(str); }
    static QString cEscape(const QString &str);
    static QString cEscape(const char *str) { return cEscape(str); }

    StateTable *table;
    CppDumpOptions options;
};

} // namespace Scxml
#endif // CPPDUMPER_H
