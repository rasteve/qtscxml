# Generated from src.pro.

add_subdirectory(scxml)
add_subdirectory(statemachine)
if(TARGET Qt::Qml)
    add_subdirectory(statemachineqml)
    add_subdirectory(scxmlqml)
    add_subdirectory(imports)
endif()
