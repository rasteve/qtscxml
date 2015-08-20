option(host_build)

TARGET = qscxmlc
CONFIG += console c++11
QT = core scxml-private

SOURCES += \
    qscxmlc.cpp \
    scxmlcppdumper.cpp

HEADERS += \
    scxmlcppdumper.h

load(qt_tool)
