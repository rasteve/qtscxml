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

#include <QCoreApplication>
#include <QFile>

#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QFile file(a.arguments().value(1));
    if (!file.open(QFile::ReadOnly)) {
        std::cout << "could not open file " << a.arguments().value(1).toStdString();
        return -1;
    }
    Scxml::ScxmlParser parser(&file);
    return a.exec();
}
