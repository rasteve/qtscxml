# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## testCpp Tool:
#####################################################################

qt_get_tool_target_name(target_name testCpp)
qt_internal_add_tool(${target_name}
    SOURCES
        testcpp.cpp
    LIBRARIES
        Qt::Qml
        Qt::Scxml
)
qt_internal_return_unless_building_tools()

# Statecharts:
qt6_add_statecharts(testCpp out.scxml)

#### Keys ignored in scope 1:.:.:testCpp.pro:<TRUE>:
# OTHER_FILES = "genTestSxcml.py" "out.scxml"
# TEMPLATE = "app"
# _OPTION = "host_build"
