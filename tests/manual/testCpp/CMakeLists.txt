# Generated from testCpp.pro.

#####################################################################
## testCpp Tool:
#####################################################################

qt_get_tool_target_name(target_name testCpp)
qt_add_tool(${target_name}
    SOURCES
        testcpp.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Scxml
)

# Statecharts:
add_qt_statecharts(testCpp FILES
    out.scxml
)

#### Keys ignored in scope 1:.:.:testCpp.pro:<TRUE>:
# OTHER_FILES = "genTestSxcml.py" "out.scxml"
# TEMPLATE = "app"
# _OPTION = "host_build"
