# Generated from qtscxml.pro.

cmake_minimum_required(VERSION 3.15.0)

project(QtScxml
    VERSION 6.0.0
    DESCRIPTION "Qt Scxml Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Qml Gui)
# special case end

qt_build_repo()
