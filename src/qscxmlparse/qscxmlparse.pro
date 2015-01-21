QT       += core qml
QT       -= gui

TARGET = qscxmlparse
CONFIG   += console c++11

TEMPLATE = app

HEADERS +=

SOURCES += \
    qscxmlparse.cpp \

include (../qscxmllib/qscxmllib.pri)
