option(host_build)
QT       += core qml qscxml

TARGET = qscxmlcpp
CONFIG   += console c++11

HEADERS +=

SOURCES += \
    qscxmlcpp.cpp

load(qt_tool)
