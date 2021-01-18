# Generated from scxml.pro.

#####################################################################
## Scxml Module:
#####################################################################

qt_add_module(Scxml
    QMAKE_MODULE_CONFIG c++11 qscxmlc
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
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:scxml.pro:<TRUE>:
# FEATURES = "../../mkspecs/features/qscxmlc.prf"
# INSTALLS = "features"
# MODULE_CONFIG = "c++11" "qscxmlc"
# features.files = "$$FEATURES"
# features.path = "$$[QT_HOST_DATA]/mkspecs/features/"

## Scopes:
#####################################################################

qt_extend_target(Scxml CONDITION QT_FEATURE_scxml_ecmascriptdatamodel
    SOURCES
        qscxmlecmascriptdatamodel.cpp qscxmlecmascriptdatamodel.h
        qscxmlecmascriptplatformproperties.cpp qscxmlecmascriptplatformproperties_p.h
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
)

#### Keys ignored in scope 3:.:.:scxml.pro:NOT force_independent AND ( NOT debug_and_release OR NOT build_all OR CONFIG(release,debug OR release) ):
# QMAKE_EXTRA_COMPILERS = "prf2build"
# prf2build.CONFIG = "no_link" "no_clean" "target_predeps"
# prf2build.commands = "$$QMAKE_COPY" "${QMAKE_FILE_IN}" "${QMAKE_FILE_OUT}"
# prf2build.input = "FEATURES"
# prf2build.name = "COPY" "${QMAKE_FILE_IN}"
# prf2build.output = "$$[QT_INSTALL_DATA/get]/mkspecs/features/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}"
qt_add_docs(Scxml
    doc/qtscxml.qdocconf
)

include(Qt6ScxmlMacros.cmake)    # special case
