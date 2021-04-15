qt_internal_add_module(StateMachineQml
    GENERATE_METATYPES
    SOURCES
        qstatemachineqmlglobals_p.h
        finalstate_p.h finalstate.cpp
        signaltransition_p.h signaltransition.cpp
        state_p.h state.cpp
        statemachine_p.h statemachine.cpp
        timeouttransition_p.h timeouttransition.cpp
        childrenprivate_p.h
        statemachineforeign_p.h
    DEFINES
        QT_BUILD_STATEMACHINEQML_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::StateMachine
)

set_target_properties(StateMachineQml PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQml.StateMachine
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQml/StateMachine"
)

qt6_qml_type_registration(StateMachineQml)
