# Generated from mediaplayer-widgets-static.pro.

cmake_minimum_required(VERSION 3.14)
project(mediaplayer-widgets-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Scxml)

add_qt_gui_executable(mediaplayer-widgets-static
    ../mediaplayer-common/mainwindow.cpp ../mediaplayer-common/mainwindow.h ../mediaplayer-common/mainwindow.ui
    mediaplayer-widgets-static.cpp
)
target_link_libraries(mediaplayer-widgets-static PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Scxml
    Qt::Widgets
)

# Statecharts:
qt6_add_statecharts(mediaplayer-widgets-static
    ../mediaplayer-common/mediaplayer.scxml
)

install(TARGETS mediaplayer-widgets-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
