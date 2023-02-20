TEMPLATE = app

QT += qml scxml
CONFIG += c++11

SOURCES += main.cpp \
    thedatamodel.cpp

HEADERS += thedatamodel.h

RESOURCES += mediaplayer.qrc

STATECHARTS = mediaplayer.scxml

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/mediaplayer
INSTALLS += target

