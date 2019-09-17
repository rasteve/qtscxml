# Generated from statemachine.pro.

#####################################################################
## tst_statemachine Test:
#####################################################################

add_qt_test(tst_statemachine
    SOURCES
        tst_statemachine.cpp
    LIBRARIES
        Qt::ScxmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Scxml
)

# Resources:
set(tst_statemachine_resource_files
    "emptylog.scxml"
    "eventoccurred.scxml"
    "historystate.scxml"
    "ids1.scxml"
    "invoke.scxml"
    "multipleinvokableservices.scxml"
    "stateDotDoneEvent.scxml"
    "statenames.scxml"
    "statenamesnested.scxml"
)

add_qt_resource(tst_statemachine "tst_statemachine"
    PREFIX
        "/tst_statemachine"
    FILES
        ${tst_statemachine_resource_files}
)


#### Keys ignored in scope 1:.:.:statemachine.pro:<TRUE>:
# TEMPLATE = "app"
