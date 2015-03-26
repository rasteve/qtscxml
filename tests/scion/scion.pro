include (../../src/qscxmllib/qscxmllib.pri)

QT += testlib
CONFIG += testcase

QT += core qml
QT -= gui

TARGET = tst_scion
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

RESOURCES = ../3rdparty/scion.qrc

SOURCES += \
    tst_scion.cpp

HEADERS += \
    $$PWD/../3rdparty/scion.h
