# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(invoke LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/invoke")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Scxml)

qt_add_executable(invoke
    invoke.cpp
)

set_target_properties(invoke PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(invoke PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Scxml
)

set(invoke_resource_files
    "Button.qml"
    "MainView.qml"
    "SubView.qml"
    "invoke.qml"
)

qt6_add_resources(invoke "invoke"
    PREFIX
        "/"
    FILES
        ${invoke_resource_files}
)

# Statecharts:
qt6_add_statecharts(invoke
    statemachine.scxml
)

install(TARGETS invoke
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
