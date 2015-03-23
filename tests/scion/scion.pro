option(host_build)

QT += testlib qscxmllib
CONFIG += testcase

QT += core qml
QT -= gui

TARGET = tst_scion
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app

RESOURCES = ../3rdparty/scion.qrc

SOURCES += \
    tst_scion.cpp

HEADERS += \
    $$PWD/../3rdparty/scion.h

load(qt_tool)
