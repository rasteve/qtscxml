include (../../src/qscxmllib/qscxmllib.pri)

QT += testlib
CONFIG += testcase

QT += core qml
QT -= gui

TARGET = tst_cppgen
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    tst_cppgen.cpp

