CONFIG += testcase
TARGET = tst_qqmlmetatype
SOURCES += tst_qqmlmetatype.cpp
macx:CONFIG -= app_bundle
QT += core-private gui-private qml-private testlib
