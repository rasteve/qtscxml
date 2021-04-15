
#####################################################################
## qtqmlstatemachine Plugin:
#####################################################################

qt_internal_add_qml_module(qtqmlstatemachine
    URI "QtQml.StateMachine"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQmlStateMachinePlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    LIBRARIES
        Qt::StateMachineQmlPrivate
)

#### Keys ignored in scope 1:.:.:statemachine.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQml/StateMachine"
