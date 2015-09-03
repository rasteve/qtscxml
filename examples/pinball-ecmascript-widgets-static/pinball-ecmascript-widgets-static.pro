QT += widgets scxml

CONFIG += c++11

STATECHARTS = ../pinball-ecmascript-common/pinball.scxml

SOURCES += \
    pinball-ecmascript-widgets-static.cpp \
    ../pinball-ecmascript-common/mainwindow.cpp

FORMS += \
    ../pinball-ecmascript-common/mainwindow.ui

HEADERS += \
    ../pinball-ecmascript-common/mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/pinball-ecmascript-widgets
INSTALLS += target

load(qscxmlc)
