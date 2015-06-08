QT += widgets qscxml

SOURCES = ../trafficlight-common/trafficlight.cpp
HEADERS = ../trafficlight-common/trafficlight.h

SOURCES += trafficlight-widgets-dynamic.cpp

DEFINES += "\"WORKING_DIR=\\\"$${PWD}\\\"\""

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlights-widgets-dynamic
target.files += statemachine.scxml
INSTALLS += target

load(qscxmlc)
