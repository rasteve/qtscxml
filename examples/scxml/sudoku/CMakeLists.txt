# Generated from sudoku.pro.

cmake_minimum_required(VERSION 3.14)
project(sudoku LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Scxml)

add_qt_gui_executable(sudoku
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_link_libraries(sudoku PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Scxml
    Qt::Widgets
)

# Resources:
set(sudoku_resource_files
    "data/nearly-solved-sudoku.data"
    "data/sudoku.data"
    "sudoku.js"
)

qt6_add_resources(sudoku "sudoku"
    PREFIX
        "/"
    FILES
        ${sudoku_resource_files}
)


# Statecharts:
qt6_add_statecharts(sudoku
    sudoku.scxml
)

install(TARGETS sudoku
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
