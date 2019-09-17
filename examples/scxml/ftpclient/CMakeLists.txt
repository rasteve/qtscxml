# Generated from ftpclient.pro.

cmake_minimum_required(VERSION 3.14)
project(ftpclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Scxml)
find_package(Qt6 COMPONENTS Network)

add_qt_gui_executable(ftpclient
    ftpcontrolchannel.cpp ftpcontrolchannel.h
    ftpdatachannel.cpp ftpdatachannel.h
    main.cpp
)
target_link_libraries(ftpclient PUBLIC
    Qt::Core
    Qt::Network
    Qt::Scxml
)

# Statecharts:
qt6_add_statecharts(ftpclient
    simpleftp.scxml
)

install(TARGETS ftpclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
