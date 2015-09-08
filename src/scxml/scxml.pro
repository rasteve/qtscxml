TARGET = QtScxml
QT = core-private qml-private
MODULE_CONFIG += c++11

load(qt_module)

CONFIG  += $$MODULE_CONFIG
DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += \
    scxmlparser.h \
    scxmlparser_p.h \
    scxmlstatemachine.h \
    scxmlstatemachine_p.h \
    scxmlglobals.h \
    nulldatamodel.h \
    ecmascriptdatamodel.h \
    ecmascriptplatformproperties_p.h \
    executablecontent.h \
    executablecontent_p.h \
    scxmlevent.h \
    scxmlevent_p.h \
    datamodel.h \
    scxmlqstates.h \
    scxmlqstates_p.h

SOURCES += \
    scxmlparser.cpp \
    scxmlstatemachine.cpp \
    nulldatamodel.cpp \
    ecmascriptdatamodel.cpp \
    ecmascriptplatformproperties.cpp \
    executablecontent.cpp \
    scxmlevent.cpp \
    datamodel.cpp \
    scxmlqstates.cpp
