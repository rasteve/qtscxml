QT += widgets qscxml

SOURCES = trafficlight.cpp trafficlight.h trafficlight-dynamic.cpp

DEFINES += "\"WORKING_DIR=\\\"$${PWD}\\\"\""

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-widgets
INSTALLS += target statemachine.scxml

load(qscxmlcpp)
