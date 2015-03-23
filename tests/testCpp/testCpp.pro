option(host_build)

QT       += core qml qscxmllib
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

load(qt_tool)
