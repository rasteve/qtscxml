TEMPLATE = app

QT += qml scxml

SOURCES += trafficlight-qml-static.cpp

RESOURCES += trafficlight-qml-static.qrc

STATECHARTS = ../trafficlight-common/statemachine.scxml

load(qscxmlc)

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-qml-static
INSTALLS += target

