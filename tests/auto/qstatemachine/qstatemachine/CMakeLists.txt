
#####################################################################
## tst_qstatemachine Test:
#####################################################################

qt_internal_add_test(tst_qstatemachine
    SOURCES
        tst_qstatemachine.cpp
    PUBLIC_LIBRARIES
        Qt::StateMachine
        Qt::StateMachinePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstatemachine CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
