#!/usr/bin/python

# Copyright (c) 2015 Digia Plc
# For any questions to Digia, please use contact form at http://qt.digia.com/
#
# All Rights Reserved.
#
# NOTICE: All information contained herein is, and remains
# the property of Digia Plc and its suppliers,
# if any. The intellectual and technical concepts contained
# herein are proprietary to Digia Plc
# and its suppliers and may be covered by Finnish and Foreign Patents,
# patents in process, and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this material
# is strictly forbidden unless prior written permission is obtained
# from Digia Plc.

from os import walk
from os.path import isfile, join, splitext

f = open("scion.qrc", "w")
f.write("<!DOCTYPE RCC><RCC version=\"1.0\">\n<qresource>\n")

g = open("scion.h","w")
g.write("const char *testBases[] = {")

first = True
mypath = "scion-tests/scxml-test-framework/test"
for root, _, filenames in walk(mypath):
    for filename in filenames:
        if filename.endswith(".scxml"):
            base = join(root,splitext(filename)[0])
            json = base+".json"
            if isfile(json):
                f.write("<file>")
                f.write(join(root,filename))
                f.write("</file>\n")
                f.write("<file>")
                f.write(json)
                f.write("</file>\n")
                if first:
                    first = False
                else:
                    g.write(",")
                g.write("\n    \"" + base + "\"")

f.write("</qresource></RCC>\n")
f.close()

g.write("\n};\n")
g.close()

