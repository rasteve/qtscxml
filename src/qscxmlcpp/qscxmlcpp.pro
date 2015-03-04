QT       += core qml
QT       -= gui

TARGET = qscxmlcpp
CONFIG   += console c++11
CONFIG -= app_bundle

TEMPLATE = app

HEADERS +=

SOURCES += \
    qscxmlcpp.cpp

include (../qscxmllib/qscxmllib.pri)
