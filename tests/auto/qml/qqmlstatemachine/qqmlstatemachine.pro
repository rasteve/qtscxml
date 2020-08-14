CONFIG += testcase
osx:CONFIG -= app_bundle

TARGET = tst_qqmlstatemachine
SOURCES += tst_qqmlstatemachine.cpp

include (../../shared/util.pri)

QT += core-private gui-private qml-private quick-private gui testlib
