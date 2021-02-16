
if(QT_BUILD_STANDALONE_TESTS)
    # Add qt_find_package calls for extra dependencies that need to be found when building
    # the standalone tests here.
    cmake_minimum_required(VERSION 3.16)
    project(QtScxmlTests VERSION 6.1.0 LANGUAGES C CXX)
    find_package(Qt6 ${PROJECT_VERSION} REQUIRED COMPONENTS BuildInternals Core Scxml StateMachine Test)
    find_package(Qt6 ${PROJECT_VERSION} OPTIONAL_COMPONENTS Qml Gui ScxmlQml StateMachineQml)
    qt_set_up_standalone_tests_build()
endif()
qt_build_tests()
