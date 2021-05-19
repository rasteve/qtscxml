cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtScxml
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Scxml and StateMachine libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Qml Widgets OpenGL OpenGLWidgets Network QuickTest)

qt_build_repo()
