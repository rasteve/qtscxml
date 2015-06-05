TARGET = QScxml
MODULE = qscxml

QT       += core core-private qml qml-private

load(qt_module)

CONFIG   += c++11
DEFINES  += SCXML_LIBRARY \
            QT_NO_CAST_FROM_ASCII

HEADERS += \
    scxmlparser.h \
    scxmlstatetable.h \
    scxmlglobals.h \
    scxmlstatetable_p.h \
    scxmlcppdumper.h \
    nulldatamodel.h \
    ecmascriptdatamodel.h \
    ecmascriptplatformproperties.h \
    executablecontent.h \
    executablecontent_p.h \
    scxmlevent.h \
    scxmlevent_p.h \
    datamodel.h

SOURCES += \
    scxmlparser.cpp \
    scxmlstatetable.cpp \
    scxmlcppdumper.cpp \
    nulldatamodel.cpp \
    ecmascriptdatamodel.cpp \
    ecmascriptplatformproperties.cpp \
    executablecontent.cpp \
    scxmlevent.cpp \
    datamodel.cpp
