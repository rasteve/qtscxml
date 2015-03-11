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
    CppDumpOptions() : usePrivateApi(false), nameQObjects(false) { }
    QString basename;
    QString namespaceName;
    bool usePrivateApi;
    bool nameQObjects;
};

class CppDumper
{
public:
    CppDumper(QTextStream &headerStream, QTextStream &cppStream, const QString &headerName, const CppDumpOptions &options)
        : h(headerStream), cpp(cppStream), headerName(headerName), options(options)
    {}

    void dump(StateTable *table);
    QTextStream &h;
    QTextStream &cpp;
    QString headerName;

private:
    void dumpConstructor();
    void dumpDeclareStates();
    void dumpDeclareTranstions();
    void dumpDeclareSignalsForEvents();
    void dumpExecutableContent();
    void dumpInstructions(ExecutableContent::Instruction &i);
    void dumpInit();

    static QByteArray b(const char *str) { return QByteArray(str); }
    static QLatin1String l (const char *str) { return QLatin1String(str); }

    QString transitionName(ScxmlTransition *transition, bool upcase = false, int tIndex = -1,
                           const QByteArray &stateName = QByteArray());

    StateTable *table;
    QString mainClassName;
    CppDumpOptions options;
};

} // namespace Scxml
#endif // CPPDUMPER_H
