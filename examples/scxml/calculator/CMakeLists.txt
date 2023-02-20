# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(calculator LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/calculator")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Scxml)

qt_add_executable(calculator
    calculator.cpp
)

set_target_properties(calculator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(calculator PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)

# Resources:
set(calculator_resource_files
    "Button.qml"
    "calculator.qml"
)

qt6_add_resources(calculator "calculator"
    PREFIX
        "/"
    FILES
        ${calculator_resource_files}
)

# Statecharts:
qt6_add_statecharts(calculator
    statemachine.scxml
)

install(TARGETS calculator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
