# Generated from mediaplayer-widgets-dynamic.pro.

cmake_minimum_required(VERSION 3.14)
project(mediaplayer-widgets-dynamic LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/scxml/mediaplayer-widgets-dynamic")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Scxml)

add_qt_gui_executable(mediaplayer-widgets-dynamic
    ../mediaplayer-common/mainwindow.cpp ../mediaplayer-common/mainwindow.h ../mediaplayer-common/mainwindow.ui
    mediaplayer-widgets-dynamic.cpp
)
target_link_libraries(mediaplayer-widgets-dynamic PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Scxml
    Qt::Widgets
)


# Resources:
set_source_files_properties("../mediaplayer-common/mediaplayer.scxml"
    PROPERTIES QT_RESOURCE_ALIAS "mediaplayer.scxml"
)
set(mediaplayer_resource_files
    "../mediaplayer-common/mediaplayer.scxml"
)

qt6_add_resources(mediaplayer-widgets-dynamic "mediaplayer"
    PREFIX
        "/"
    FILES
        ${mediaplayer_resource_files}
)

install(TARGETS mediaplayer-widgets-dynamic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
