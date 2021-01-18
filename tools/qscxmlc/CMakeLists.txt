# Generated from qscxmlc.pro.

#####################################################################
## qscxmlc Tool:
#####################################################################

qt_get_tool_target_name(target_name qscxmlc)
qt_add_tool(${target_name}
    TOOLS_TARGET Scxml # special case
    SOURCES
        ../../src/scxml/qscxmlcompiler.cpp ../../src/scxml/qscxmlcompiler.h ../../src/scxml/qscxmlcompiler_p.h
        ../../src/scxml/qscxmlerror.cpp ../../src/scxml/qscxmlerror.h
        ../../src/scxml/qscxmlexecutablecontent.cpp ../../src/scxml/qscxmlexecutablecontent.h ../../src/scxml/qscxmlexecutablecontent_p.h
        ../../src/scxml/qscxmlglobals.h
        ../../src/scxml/qscxmltabledata.cpp ../../src/scxml/qscxmltabledata.h
        generator.cpp generator.h
        main.cpp
        moc.h
        outputrevision.h
        qscxmlc.cpp qscxmlc.h
        scxmlcppdumper.cpp scxmlcppdumper.h
        utils.h
    DEFINES
        BUILD_QSCXMLC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
# special case begin
        $<TARGET_PROPERTY:Qt::Scxml,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:Qt::ScxmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
# special case end
    LIBRARIES
        Qt::CorePrivate
)

# special case begin
set_property(SOURCE ../../src/scxml/qscxmlerror.h PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE ../../src/scxml/qscxmlcompiler.cpp PROPERTY SKIP_AUTOMOC ON)
# special case end

# Resources:
set(templates_resource_files
    "cppdatamodel.t"
    "data.t"
    "decl.t"
)

qt_add_resource(${target_name} "templates"
    PREFIX
        "/"
    FILES
        ${templates_resource_files}
    OPTIONS --no-compress # special case
)


#### Keys ignored in scope 1:.:.:qscxmlc.pro:<TRUE>:
# _OPTION = "host_build"
