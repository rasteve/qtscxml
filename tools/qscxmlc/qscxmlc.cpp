/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtScxml/private/qscxmlparser_p.h>
#include <QtScxml/qscxmltabledata.h>
#include "scxmlcppdumper.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

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

static void collectAllDocuments(DocumentModel::ScxmlDocument *doc, QMap<DocumentModel::ScxmlDocument *, QString> *docs)
{
    docs->insert(doc, doc->root->name);
    foreach (DocumentModel::ScxmlDocument *subDoc, doc->allSubDocuments) {
        collectAllDocuments(subDoc, docs);
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationVersion(QString::fromLatin1("%1 (Qt %2)").arg(
                            QString::number(Q_QSCXMLC_OUTPUT_REVISION),
                            QString::fromLatin1(QT_VERSION_STR)));
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
    QScxmlParser parser(&reader);
    parser.setFileName(file.fileName());
    parser.parse();
    if (!parser.errors().isEmpty()) {
        foreach (const QScxmlError &error, parser.errors()) {
            errs << error.toString() << endl;
        }
        return ParseError;
    }

    auto mainDoc = QScxmlParserPrivate::get(&parser)->scxmlDocument();
    if (mainDoc == nullptr) {
        Q_ASSERT(!parser.errors().isEmpty());
        foreach (const QScxmlError &error, parser.errors()) {
            errs << error.toString() << endl;
        }
        return ScxmlVerificationError;
    }

    QMap<DocumentModel::ScxmlDocument *, QString> docs;
    collectAllDocuments(mainDoc, &docs);
    if (mainClassname.isEmpty())
        mainClassname = mainDoc->root->name;
    if (mainClassname.isEmpty()) {
        mainClassname = QFileInfo(scxmlFileName).fileName();
        int dot = mainClassname.indexOf(QLatin1Char('.'));
        if (dot != -1)
            mainClassname = mainClassname.left(dot);
    }
    docs.insert(mainDoc, mainClassname);

    TranslationUnit tu = options;
    tu.scxmlFileName = QFileInfo(file).fileName();
    tu.mainDocument = mainDoc;
    tu.outHFileName = outHFileName;
    tu.outCppFileName = outCppFileName;
    for (QMap<DocumentModel::ScxmlDocument *, QString>::const_iterator i = docs.begin(), ei = docs.end(); i != ei; ++i) {
        auto name = i.value();
        if (name.isEmpty()) {
            name = QStringLiteral("%1_StateMachine_%2").arg(mainClassname).arg(tu.classnameForDocument.size() + 1);
        }
        tu.classnameForDocument.insert(i.key(), name);
    }

    int err = write(&tu);

    return err;
}
