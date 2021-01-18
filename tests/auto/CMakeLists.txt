# Generated from auto.pro.

# special case begin
# add_subdirectory(cmake)
# special case end
add_subdirectory(compiled)
add_subdirectory(dynamicmetaobject)
add_subdirectory(parser)
add_subdirectory(scion)
add_subdirectory(statemachine)
add_subdirectory(statemachineinfo)
add_subdirectory(qml)
add_subdirectory(qmltest)
if(NOT UIKIT)
    add_subdirectory(qstatemachine)
endif()
