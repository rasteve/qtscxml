option(host_build)
QT       += core qml qscxmllib
QT       -= gui network

TARGET = qscxmlserver
CONFIG   += console c++11

HEADERS +=

SOURCES += \
    qscxmlserver.cpp

load(qt_tool)
