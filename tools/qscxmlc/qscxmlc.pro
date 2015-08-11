option(host_build)
QT += core qml scxml-private

TARGET = qscxmlc
CONFIG += console c++11

SOURCES += \
    qscxmlc.cpp \
    scxmlcppdumper.cpp

HEADERS += \
    scxmlcppdumper.h

load(qt_tool)
