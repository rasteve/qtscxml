qt_internal_add_module(ScxmlQml
    GENERATE_METATYPES
    SOURCES
        eventconnection.cpp eventconnection_p.h
        invokedservices.cpp invokedservices_p.h
        statemachineextended.cpp statemachineextended_p.h
        statemachineloader.cpp statemachineloader_p.h
        qscxmlqmlglobals_p.h
    DEFINES
        QT_BUILD_SCXMLQML_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Scxml
)

set_target_properties(ScxmlQml PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtScxml
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtScxml"
)

qt6_qml_type_registration(ScxmlQml)
