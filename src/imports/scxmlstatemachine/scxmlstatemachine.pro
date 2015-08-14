CXX_MODULE = qml
TARGET = scxmlstatemachine
TARGETPATH = Scxml
IMPORT_VERSION = 1.0

QT = scxml qml-private core-private

SOURCES = \
    $$PWD/plugin.cpp \
    $$PWD/statemachineloader.cpp

HEADERS = \
    $$PWD/statemachineloader.h

load(qml_plugin)
