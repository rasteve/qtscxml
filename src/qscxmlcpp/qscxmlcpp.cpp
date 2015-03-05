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

#include "../qscxmllib/scxmlparser.h"
#include "../qscxmllib/scxmlcppdumper.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString fileName = a.arguments().value(1);
    if (fileName.isEmpty()) {
        std::cout << "no filename given:"
                  << QFileInfo(a.arguments().value(0)).completeBaseName().toStdString()
                  << " file.scxml" << std::endl;
        return -1;
    }
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        std::cout << "Error: could not open input file " << fileName.toStdString();
        return -2;
    }

    QXmlStreamReader reader(&file);
    Scxml::ScxmlParser parser(&reader,
                              Scxml::ScxmlParser::loaderForDir(QFileInfo(file.fileName()).absolutePath()));
    parser.parse();

    QFile outH(a.arguments().value(2, QFileInfo(fileName).baseName() + QLatin1String(".h")));
    if (!outH.open(QFile::WriteOnly)) {
        std::cerr << "Error: cannot open " << outH.fileName().toStdString()
                  << ": " << outH.errorString().toStdString();
        return -3;
    }

    QFile outCpp(a.arguments().value(2, QFileInfo(fileName).baseName() + QLatin1String(".cpp")));
    if (!outCpp.open(QFile::WriteOnly)) {
        std::cerr << "Error: cannot open " << outCpp.fileName().toStdString()
                  << ": " << outCpp.errorString().toStdString();
        return -4;
    }

    QTextStream h(&outH);
    QTextStream c(&outCpp);
    Scxml::CppDumpOptions options;
    Scxml::CppDumper dumper(h, c, outH.fileName(), options);
    dumper.dump(parser.table());
    outH.close();
    outCpp.close();

    a.exit();
    return 0;
}
