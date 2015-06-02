TEMPLATE = app

QT += qml qscxmllib

SOURCES += trafficlight-qml-static.cpp

RESOURCES += trafficlight-qml-static.qrc

STATECHARTS = statemachine.scxml

load(qscxmlcpp)

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-widgets
INSTALLS += target

