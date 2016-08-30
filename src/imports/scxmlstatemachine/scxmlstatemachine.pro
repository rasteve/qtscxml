TARGET = scxml
TARGETPATH = QtScxml

QT = scxml qml-private core-private

SOURCES = \
    $$PWD/plugin.cpp \
    $$PWD/statemachineloader.cpp \
    $$PWD/eventconnection.cpp \
    $$PWD/statemachineextended.cpp \
    $$PWD/invokedservices.cpp

HEADERS = \
    $$PWD/statemachineloader.h \
    $$PWD/eventconnection.h \
    $$PWD/statemachineextended.h \
    $$PWD/invokedservices.h

load(qml_plugin)

OTHER_FILES += plugins.qmltypes qmldir
