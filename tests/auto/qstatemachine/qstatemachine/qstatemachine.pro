CONFIG += testcase
TARGET = tst_qstatemachine
QT = core statemachine statemachine-private testlib
qtHaveModule(widgets): QT += widgets
SOURCES = tst_qstatemachine.cpp
