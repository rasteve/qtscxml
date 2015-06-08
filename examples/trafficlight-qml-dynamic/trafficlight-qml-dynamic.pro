TEMPLATE = app

QT += qml qscxml

SOURCES += trafficlight-qml-dynamic.cpp

RESOURCES += trafficlight-qml-dynamic.qrc

load(qscxmlc)

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-qml-dynamic
INSTALLS += target

