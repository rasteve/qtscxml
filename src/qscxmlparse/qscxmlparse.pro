option(host_build)
QT       += core qml qscxmllib

TARGET = qscxmlparse
CONFIG   += console c++11

HEADERS +=

SOURCES += \
    qscxmlparse.cpp

load(qt_tool)
