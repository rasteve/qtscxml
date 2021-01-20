CONFIG += testcase
TARGET = tst_qqmlstatemachinemetatype
SOURCES += tst_qqmlstatemachinemetatype.cpp
macx:CONFIG -= app_bundle
QT += core-private gui-private qml-private testlib
