QT += qml scxml

CONFIG += c++11

SOURCES += calculator.cpp

RESOURCES += calculator.qrc

STATECHARTS = statemachine.scxml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/calculator
INSTALLS += target
