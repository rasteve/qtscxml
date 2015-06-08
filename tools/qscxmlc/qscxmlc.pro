option(host_build)
QT += core qml qscxml

TARGET = qscxmlc
CONFIG += console c++11

SOURCES += qscxmlc.cpp

load(qt_tool)
