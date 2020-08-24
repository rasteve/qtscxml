!qtHaveModule(gui)): return()

qtConfig(qeventtransition) {
    QT += gui
    QT_FOR_PRIVATE += gui-private
    HEADERS += \
               $$PWD/qkeyeventtransition.h \
               $$PWD/qmouseeventtransition.h \
               $$PWD/qbasickeyeventtransition_p.h \
               $$PWD/qbasicmouseeventtransition_p.h
    SOURCES += \
               $$PWD/qkeyeventtransition.cpp \
               $$PWD/qmouseeventtransition.cpp \
               $$PWD/qbasickeyeventtransition.cpp \
               $$PWD/qbasicmouseeventtransition.cpp
}
