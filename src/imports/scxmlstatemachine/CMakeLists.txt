

#####################################################################
## scxml Plugin:
#####################################################################

qt_internal_add_qml_module(declarative_scxml
    URI "QtScxml"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QScxmlStateMachinePlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    LIBRARIES
        Qt::ScxmlQmlPrivate
)

#### Keys ignored in scope 1:.:.:scxmlstatemachine.pro:<TRUE>:
# IMPORT_VERSION = "5.$$QT_MINOR_VERSION"
# OTHER_FILES = "plugins.qmltypes" "qmldir"
# TARGETPATH = "QtScxml"
