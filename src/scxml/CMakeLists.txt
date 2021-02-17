
#####################################################################
## Scxml Module:
#####################################################################

qt_internal_add_module(Scxml
    QMAKE_MODULE_CONFIG c++11 qscxmlc
    PLUGIN_TYPES scxmldatamodel
    SOURCES
        qscxmlcompiler.cpp qscxmlcompiler.h qscxmlcompiler_p.h
        qscxmlcppdatamodel.cpp qscxmlcppdatamodel.h qscxmlcppdatamodel_p.h
        qscxmldatamodel.cpp qscxmldatamodel.h qscxmldatamodel_p.h
        qscxmlerror.cpp qscxmlerror.h
        qscxmlevent.cpp qscxmlevent.h qscxmlevent_p.h
        qscxmlexecutablecontent.cpp qscxmlexecutablecontent.h qscxmlexecutablecontent_p.h
        qscxmlglobals.h qscxmlglobals_p.h
        qscxmlinvokableservice.cpp qscxmlinvokableservice.h qscxmlinvokableservice_p.h
        qscxmlnulldatamodel.cpp qscxmlnulldatamodel.h
        qscxmlstatemachine.cpp qscxmlstatemachine.h qscxmlstatemachine_p.h
        qscxmlstatemachineinfo.cpp qscxmlstatemachineinfo_p.h
        qscxmltabledata.cpp qscxmltabledata.h qscxmltabledata_p.h
        qscxmldatamodelplugin.h qscxmldatamodelplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

# This allows QML element registration outside of this module with the "QML_ELEMENT" & friends
qt6_extract_metatypes(Scxml)

#### Keys ignored in scope 1:.:.:scxml.pro:<TRUE>:
# FEATURES = "../../mkspecs/features/qscxmlc.prf"
# INSTALLS = "features"
# MODULE_CONFIG = "c++11" "qscxmlc"
# features.files = "$$FEATURES"
# features.path = "$$[QT_HOST_DATA]/mkspecs/features/"

## Scopes:
#####################################################################


#### Keys ignored in scope 3:.:.:scxml.pro:NOT force_independent AND ( NOT debug_and_release OR NOT build_all OR CONFIG(release,debug OR release) ):
# QMAKE_EXTRA_COMPILERS = "prf2build"
# prf2build.CONFIG = "no_link" "no_clean" "target_predeps"
# prf2build.commands = "$$QMAKE_COPY" "${QMAKE_FILE_IN}" "${QMAKE_FILE_OUT}"
# prf2build.input = "FEATURES"
# prf2build.name = "COPY" "${QMAKE_FILE_IN}"
# prf2build.output = "$$[QT_INSTALL_DATA/get]/mkspecs/features/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}"
qt_internal_add_docs(Scxml
    doc/qtscxml.qdocconf
)

include(Qt6ScxmlMacros.cmake)
