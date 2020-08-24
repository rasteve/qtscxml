!qtConfig(statemachine): return()

TARGET = QtStateMachine
QT = core
CONFIG += c++11

QT_FOR_PRIVATE = core-private
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

include(statemachine.pri)
include(gui/statemachine.pri)
HEADERS += qstatemachineglobal.h

load(qt_module)
