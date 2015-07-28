QT += widgets qscxml

CONFIG += c++11

SOURCES = ../trafficlight-common/trafficlight.cpp
HEADERS = ../trafficlight-common/trafficlight.h

SOURCES += trafficlight-widgets-dynamic.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/trafficlights-widgets-dynamic
INSTALLS += target

load(qscxmlc)

RESOURCES += \
    trafficlight-widgets-dynamic.qrc
