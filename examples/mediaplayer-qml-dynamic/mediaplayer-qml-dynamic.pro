TEMPLATE = app

QT += qml scxml

SOURCES += mediaplayer-qml-dynamic.cpp

RESOURCES += mediaplayer-qml-dynamic.qrc

load(qscxmlc)

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/mediaplayer-qml-dynamic
INSTALLS += target
