option(host_build)
QT += core qml qscxml-private

TARGET = qscxmlc
CONFIG += console c++11

SOURCES += \
    qscxmlc.cpp \
    scxmlcppdumper.cpp

HEADERS += \
    scxmlcppdumper.h

load(qt_tool)
