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

#include "../qscxmllib/scxmlparser.h"
#include "../qscxmllib/scxmldumper.h"

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
        std::cout << "could not open file " << fileName.toStdString();
        return -1;
    }
    QXmlStreamReader reader(&file);
    Scxml::ScxmlParser parser(&reader,
                              Scxml::ScxmlParser::loaderForDir(QFileInfo(file.fileName()).absolutePath()));
    parser.parse();
    QFile outF(a.arguments().value(2, QLatin1String("out.scxml")));
    outF.open(QFile::WriteOnly);
    QXmlStreamWriter w(&outF);
    w.setAutoFormattingIndent(2);
    w.setAutoFormatting(true);
    Scxml::ScxmlDumper dumper(w);
    dumper.dump(parser.table());
    outF.close();
    a.exit();
    return a.exec();
}
