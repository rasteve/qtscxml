QT += widgets scxml

SOURCES = ../trafficlight-common/trafficlight.cpp
HEADERS = ../trafficlight-common/trafficlight.h
STATECHARTS = ../trafficlight-common/statemachine.scxml

load(qscxmlc)

SOURCES += trafficlight-widgets-static.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-widgets-static
INSTALLS += target
