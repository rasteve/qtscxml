TEMPLATE = app

QT += qml scxml

SOURCES += mediaplayer-qml-static.cpp

RESOURCES += mediaplayer-qml-static.qrc

STATECHARTS = ../mediaplayer-common/mediaplayer.scxml

load(qscxmlc)

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/mediaplayer-qml-static
INSTALLS += target

