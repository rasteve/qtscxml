QT += widgets scxml

CONFIG += c++11

STATECHARTS = ../pinball-common/pinball.scxml

SOURCES += \
    pinball-widgets-static.cpp \
    ../pinball-common/mainwindow.cpp

FORMS += \
    ../pinball-common/mainwindow.ui

HEADERS += \
    ../pinball-common/mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/pinball-widgets
INSTALLS += target

load(qscxmlc)
