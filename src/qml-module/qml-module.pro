CXX_MODULE = qml
TARGET = scxmlstatemachine
TARGETPATH = Scxml
IMPORT_VERSION = 1.0

QT = qscxmllib #qml-private

SOURCES = \
    $$PWD/plugin.cpp \
    $$PWD/state.cpp \
    $$PWD/statemachine.cpp

HEADERS = \
    $$PWD/state.h \
    $$PWD/statemachine.h

load(qml_plugin)
