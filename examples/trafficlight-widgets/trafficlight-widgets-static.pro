QT += widgets qscxmllib

SOURCES = trafficlight.cpp trafficlight.h trafficlight-static.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlight-widgets
INSTALLS += target

STATECHARTS = statemachine.scxml

load(qscxmlcpp)
