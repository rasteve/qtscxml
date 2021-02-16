qt_internal_add_test(tst_scxmlqmlcpp
    SOURCES
        tst_scxmlqmlcpp.cpp
    LIBRARIES
        Qt::Scxml
        Qt::ScxmlQmlPrivate
        Qt::CorePrivate
)
