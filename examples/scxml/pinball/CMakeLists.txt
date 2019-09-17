# Generated from pinball.pro.

cmake_minimum_required(VERSION 3.14)
project(pinball LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Scxml)

add_qt_gui_executable(pinball
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
)
target_link_libraries(pinball PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Scxml
    Qt::Widgets
)

# Statecharts:
qt6_add_statecharts(pinball
    pinball.scxml
)

install(TARGETS pinball
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
