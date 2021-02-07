qt_internal_add_test(tst_scxmlcoutput_path
    SOURCES
        tst_scxmlcoutput.cpp
    PUBLIC_LIBRARIES
        Qt::Scxml
)

# This testcase tests providing the namespace
qt6_add_statecharts(tst_scxmlcoutput_path
    ../shared/ids1.scxml
    OUTPUT_DIR thechosenpath
)
