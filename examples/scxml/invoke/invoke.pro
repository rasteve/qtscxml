TEMPLATE = app

QT += qml scxml
CONFIG += c++11

SOURCES += invoke.cpp

RESOURCES += invoke.qrc

STATECHARTS = statemachine.scxml

target.path = $$[QT_INSTALL_EXAMPLES]/scxml/invoke
INSTALLS += target

