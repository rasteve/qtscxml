# Generated from scxmlstatemachine.pro.

#####################################################################
## scxml Plugin:
#####################################################################

qt_internal_add_qml_module(declarative_scxml # special case
    URI "QtScxml"
    VERSION "5.${CMAKE_PROJECT_VERSION_MINOR}"
    CLASSNAME QScxmlStateMachinePlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        eventconnection.cpp eventconnection_p.h
        invokedservices.cpp invokedservices_p.h
        plugin.cpp
        statemachineextended.cpp statemachineextended_p.h
        statemachineloader.cpp statemachineloader_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::Scxml
)

#### Keys ignored in scope 1:.:.:scxmlstatemachine.pro:<TRUE>:
# IMPORT_VERSION = "5.$$QT_MINOR_VERSION"
# OTHER_FILES = "plugins.qmltypes" "qmldir"
# TARGETPATH = "QtScxml"
