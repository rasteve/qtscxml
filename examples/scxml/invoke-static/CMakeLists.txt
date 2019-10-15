# Generated from invoke-static.pro.

cmake_minimum_required(VERSION 3.14)
project(invoke-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/scxml/invoke-static")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Scxml)

add_qt_gui_executable(invoke-static
    invoke-static.cpp
)
target_link_libraries(invoke-static PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)


# Resources:
set_source_files_properties("../invoke-common/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../invoke-common/MainView.qml"
    PROPERTIES QT_RESOURCE_ALIAS "MainView.qml"
)
set_source_files_properties("../invoke-common/SubView.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SubView.qml"
)
set(invoke-static_resource_files
    "../invoke-common/Button.qml"
    "../invoke-common/MainView.qml"
    "../invoke-common/SubView.qml"
    "invoke-static.qml"
)

qt6_add_resources(invoke-static "invoke-static"
    PREFIX
        "/"
    FILES
        ${invoke-static_resource_files}
)


# Statecharts:
qt6_add_statecharts(invoke-static
    ../invoke-common/statemachine.scxml
)

install(TARGETS invoke-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
