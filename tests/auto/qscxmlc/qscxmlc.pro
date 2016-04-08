QT = core testlib scxml-private
CONFIG += testcase console c++11
CONFIG -= app_bundle

TARGET = tst_qscxmlc
TEMPLATE = app

RESOURCES += tst_qscxmlc.qrc

SOURCES += \
    $$PWD/tst_qscxmlc.cpp \
    $$PWD/../../../tools/qscxmlc/generator.cpp \
    $$PWD/../../../tools/qscxmlc/scxmlcppdumper.cpp

HEADERS += \
    $$PWD/../../../tools/qscxmlc/qscxmlc.cpp \ # yes, that's a header. We want to #define scxmlcmain main
    $$PWD/../../../tools/qscxmlc/moc.h \
    $$PWD/../../../tools/qscxmlc/generator.h \
    $$PWD/../../../tools/qscxmlc/outputrevision.h \
    $$PWD/../../../tools/qscxmlc/utils.h \
    $$PWD/../../../tools/qscxmlc/scxmlcppdumper.h

INCLUDEPATH += ../../../tools/qscxmlc/

