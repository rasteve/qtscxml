# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(mediaplayer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Scxml)

qt_add_executable(mediaplayerscxml
    main.cpp
    thedatamodel.cpp thedatamodel.h
)

set_target_properties(mediaplayerscxml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(mediaplayerscxml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)

set(mediaplayer_resource_files
    "Mediaplayer.qml"
)

qt6_add_resources(mediaplayerscxml "mediaplayer"
    PREFIX
        "/"
    FILES
        ${mediaplayer_resource_files}
)

# Statecharts:
qt6_add_statecharts(mediaplayerscxml
    mediaplayer.scxml
)

install(TARGETS mediaplayerscxml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
