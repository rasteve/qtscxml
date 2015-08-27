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

#include <QtScxml/private/scxmlparser_p.h>
#include "scxmlcppdumper.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

using namespace Scxml;

enum {
    NoError = 0,
    CommandLineArgumentsError = -1,
    NoInputFilesError = -2,
    CannotOpenInputFileError = -3,
    ParseError = -4,
    CannotOpenOutputHeaderFileError = -5,
    CannotOpenOutputCppFileError = -6,
    ScxmlVerificationError = -7
};

int write(TranslationUnit *tu)
{
    QTextStream errs(stderr, QIODevice::WriteOnly);

    QFile outH(tu->outHFileName);
    if (!outH.open(QFile::WriteOnly)) {
        errs << QStringLiteral("Error: cannot open '%1': %2").arg(outH.fileName(), outH.errorString()) << endl;
        exit(CannotOpenOutputHeaderFileError);
    }

    QFile outCpp(tu->outCppFileName);
    if (!outCpp.open(QFile::WriteOnly)) {
        errs << QStringLiteral("Error: cannot open '%1': %2").arg(outCpp.fileName(), outCpp.errorString()) << endl;
        exit(CannotOpenOutputCppFileError);
    }

    QTextStream h(&outH);
    QTextStream c(&outCpp);
    CppDumper dumper(h, c);
    dumper.dump(tu);
    outH.close();
    outCpp.close();
    return NoError;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList args = a.arguments();
    QString usage = QStringLiteral("\nUsage: %1 [-no-c++11] [-namespace <namespace>] [-o <base/out/name>] [-oh <header/out>] [-ocpp <cpp/out>] [-use-private-api]\n").arg(QFileInfo(args.value(0)).baseName());
           usage += QStringLiteral("         [-classname <stateMachineClassName>] <input.scxml>\n\n");
           usage += QStringLiteral("compiles the given input.scxml file to a header and cpp file\n");

    QTextStream errs(stderr, QIODevice::WriteOnly);

    TranslationUnit options;
    QString scxmlFileName;
    QString outFileName;
    QString outHFileName;
    QString outCppFileName;
    QString mainClassname;
    for (int iarg = 1; iarg < args.size(); ++iarg) {
        QString arg = args.at(iarg);
        if (arg == QStringLiteral("-no-c++11")) {
            options.useCxx11 = false;
        } else if (arg == QLatin1String("-namespace")) {
            options.namespaceName = args.value(++iarg);
        } else if (arg == QLatin1String("-o")) {
            outFileName = args.value(++iarg);
        } else if (arg == QLatin1String("-oh")) {
            outHFileName = args.value(++iarg);
        } else if (arg == QLatin1String("-ocpp")) {
            outCppFileName = args.value(++iarg);
        } else if (arg == QLatin1String("-classname")) {
            mainClassname = args.value(++iarg);
        } else if (scxmlFileName.isEmpty()) {
            scxmlFileName = arg;
        } else {
            errs << QStringLiteral("Unexpected argument: %1").arg(arg) << endl;
            errs << usage;
            return CommandLineArgumentsError;
        }
    }
    if (scxmlFileName.isEmpty()) {
        errs << QStringLiteral("Error: no input files.") << endl;
        exit(NoInputFilesError);
    }
    QFile file(scxmlFileName);
    if (!file.open(QFile::ReadOnly)) {
        errs << QStringLiteral("Error: cannot open input file %1").arg(scxmlFileName);
        exit(CannotOpenInputFileError);
    }
    if (outFileName.isEmpty())
        outFileName = QFileInfo(scxmlFileName).baseName();
    if (outHFileName.isEmpty())
        outHFileName = outFileName + QLatin1String(".h");
    if (outCppFileName.isEmpty())
        outCppFileName = outFileName + QLatin1String(".cpp");

    QXmlStreamReader reader(&file);
    ScxmlParser parser(&reader);
    parser.setFileName(file.fileName());
    parser.parse();
    if (!parser.errors().isEmpty()) {
        foreach (const ScxmlError &error, parser.errors()) {
            errs << error.toString() << endl;
        }
        return ParseError;
    }

    auto mainDoc = ScxmlParserPrivate::get(&parser)->scxmlDocument();
    if (mainDoc == nullptr) {
        Q_ASSERT(!parser.errors().isEmpty());
        foreach (const ScxmlError &error, parser.errors()) {
            errs << error.toString() << endl;
        }
        return ScxmlVerificationError;
    }

    struct : public DocumentModel::NodeVisitor {
        bool visit(DocumentModel::Invoke *invoke) Q_DECL_OVERRIDE {
            if (DocumentModel::ScxmlDocument *doc = invoke->content.data()) {
                docs.insert(doc, doc->root->name);
            }
            return true;
        }

        QMap<DocumentModel::ScxmlDocument *, QString> docs;
    } collector;
    mainDoc->root->accept(&collector);
    if (mainClassname.isEmpty())
        mainClassname = mainDoc->root->name;
    collector.docs.insert(mainDoc, mainClassname);

    TranslationUnit tu = options;
    tu.mainDocument = mainDoc;
    tu.outHFileName = outHFileName;
    tu.outCppFileName = outCppFileName;
    for (QMap<DocumentModel::ScxmlDocument *, QString>::const_iterator i = collector.docs.begin(), ei = collector.docs.end(); i != ei; ++i) {
        auto name = i.value();
        if (name.isEmpty()) {
            name = QStringLiteral("StateMachine_%1").arg(tu.classnameForDocument.size() + 1);
        }
        tu.classnameForDocument.insert(i.key(), name);
    }

    int err = write(&tu);

    return err;
}
