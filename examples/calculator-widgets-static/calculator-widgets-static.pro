QT += widgets scxml

CONFIG += c++11

STATECHARTS = ../calculator-common/calculator.scxml

SOURCES += \
    calculator-widgets-static.cpp \
    ../calculator-common/mainwindow.cpp

FORMS += \
    ../calculator-common/mainwindow.ui

HEADERS += \
    ../calculator-common/mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/calculator-widgets
INSTALLS += target

load(qscxmlc)
