QT       += core qml
QT       -= gui

TARGET = testCpp
CONFIG   += console c++11
CONFIG -= app_bundle

TEMPLATE = app

HEADERS += \
    out.h

SOURCES += \
    testcpp.cpp \
    out.cpp

OTHER_FILES += genTestSxcml.py

include (../../src/qscxmllib/qscxmllib.pri)
