# Generated from statemachineinfo.pro.

#####################################################################
## tst_statemachineinfo Test:
#####################################################################

qt_add_test(tst_statemachineinfo
    SOURCES
        tst_statemachineinfo.cpp
    LIBRARIES
        Qt::ScxmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Scxml
)

# Resources:
set(tst_statemachineinfo_resource_files
    "statemachine.scxml"
)

qt_add_resource(tst_statemachineinfo "tst_statemachineinfo"
    PREFIX
        "/tst_statemachineinfo"
    FILES
        ${tst_statemachineinfo_resource_files}
)


#### Keys ignored in scope 1:.:.:statemachineinfo.pro:<TRUE>:
# TEMPLATE = "app"
