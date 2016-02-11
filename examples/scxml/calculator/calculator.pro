QT += widgets scxml

CONFIG += c++11

STATECHARTS = calculator.scxml

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scxml/calculator
INSTALLS += target

load(qscxmlc)
