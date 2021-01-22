# Generated from examples.pro.

qt_examples_build_begin()

if(TARGET Qt::Scxml)
    add_subdirectory(scxml)
endif()
if(TARGET Qt::StateMachine AND QT_FEATURE_qeventtransition)
    add_subdirectory(widgets)
endif()

qt_examples_build_end()
