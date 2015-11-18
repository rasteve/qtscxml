option(host_build)

TARGET = qscxmlc
CONFIG += console c++11
QT = core

SOURCES += \
    qscxmlc.cpp \
    scxmlcppdumper.cpp

HEADERS += \
    scxmlcppdumper.h

HEADERS += \
    $$PWD/../../src/scxml/qscxmlparser.h \
    $$PWD/../../src/scxml/qscxmlparser_p.h \
    $$PWD/../../src/scxml/qscxmlglobals.h \
    $$PWD/../../src/scxml/qscxmlexecutablecontent.h \
    $$PWD/../../src/scxml/qscxmlexecutablecontent_p.h \
    $$PWD/../../src/scxml/qscxmlerror.h \
    $$PWD/../../src/scxml/qscxmltabledata.h

SOURCES += \
    $$PWD/../../src/scxml/qscxmlparser.cpp \
    $$PWD/../../src/scxml/qscxmlexecutablecontent.cpp \
    $$PWD/../../src/scxml/qscxmlerror.cpp \
    $$PWD/../../src/scxml/qscxmltabledata.cpp

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
INCLUDEPATH *= $$QT.scxml.includes $$QT.scxml_private.includes

load(qt_tool)
