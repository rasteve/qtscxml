# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_cppgen Tool:
#####################################################################

qt_get_tool_target_name(target_name tst_cppgen)
qt_internal_add_tool(${target_name}
    SOURCES
        tst_cppgen.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Scxml
        Qt::Test
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:cppgen.pro:<TRUE>:
# TEMPLATE = "app"
# _OPTION = "host_build"
