CXX_MODULE = qml
TARGET = scxmlstatemachine
TARGETPATH = Scxml
IMPORT_VERSION = 1.0

QT = scxml qml-private core-private

SOURCES = \
    $$PWD/plugin.cpp \
    $$PWD/signalevent.cpp \
    $$PWD/state.cpp \
    $$PWD/statemachine.cpp

HEADERS = \
    $$PWD/signalevent.h \
    $$PWD/state.h \
    $$PWD/statemachine.h

load(qml_plugin)
