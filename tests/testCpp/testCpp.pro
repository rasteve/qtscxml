QT       += core qml
QT       -= gui

TARGET = qscxmlparse
CONFIG   += console c++11

TEMPLATE = app

HEADERS += \
    out.h

SOURCES += \
    testcpp.cpp

OTHER_FILES += genTestSxcml.py

include (../../src/qscxmllib/qscxmllib.pri)
