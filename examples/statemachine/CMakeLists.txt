# Generated from widgets.pro.

if(NOT TARGET Qt::Widgets)
    return()
endif()
add_subdirectory(graphicsview)
add_subdirectory(statemachine)
if(QT_FEATURE_animation)
    add_subdirectory(animation)
endif()
