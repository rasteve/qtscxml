QT       += core qml
QT       -= gui network

TARGET = qscxmlserver
CONFIG   += console c++11

TEMPLATE = app

HEADERS +=

SOURCES += \
    qscxmlserver.cpp \

include (../qscxmllib/qscxmllib.pri)
