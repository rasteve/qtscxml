QT += testlib qscxmllib
CONFIG += testcase

QT += core qml
QT -= gui

TARGET = tst_scion
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app

RESOURCES = ../3rdparty/scion.qrc

SOURCES += \
    tst_scion.cpp

HEADERS += \
    $$PWD/../3rdparty/scion.h

defineReplace(nameTheNamespace) {
    sn=$$relative_path($$absolute_path($$dirname(1), $$OUT_PWD),$$MYSCXMLS_DIR)
    sn~=s/\\.txml$//
    sn~=s/[^a-zA-Z_0-9]/_/
    return ($$sn)
}
defineReplace(nameTheClass) {
    cn = $$basename(1)
    cn ~= s/\\.scxml$//
    cn ~=s/\\.txml$//
    cn ~= s/[^a-zA-Z_0-9]/_/
    return ($$cn)
}

myscxml.commands = $$[QT_HOST_BINS]/qscxmlcpp -name-qobjects -oh scxml/${QMAKE_FUNC_nameTheNamespace}_${QMAKE_FILE_IN_BASE}.h -ocpp ${QMAKE_FILE_OUT} -namespace ${QMAKE_FUNC_nameTheNamespace} -classname ${QMAKE_FUNC_nameTheClass} ${QMAKE_FILE_IN}
myscxml.depends += $$[QT_HOST_BINS]/qscxmlcpp
myscxml.output = scxml/${QMAKE_FUNC_nameTheNamespace}_${QMAKE_FILE_IN_BASE}.cpp
myscxml.input = MYSCXMLS
myscxml.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += myscxml

MYSCXMLS_DIR += $$absolute_path($$PWD/../3rdparty/scion-tests/scxml-test-framework/test)
MYSCXMLS = $$files($$MYSCXMLS_DIR/*.scxml, true)

for (f,MYSCXMLS) {
    cn = $$basename(f)
    cn ~= s/\\.scxml$//
    cn ~=s/\\.txml$//
    sn = $$relative_path($$dirname(f), $$MYSCXMLS_DIR)
    sn ~=s/[^a-zA-Z_0-9]/_/

    inc_list += "$${LITERAL_HASH}include \"scxml/$${sn}_$${cn}.h\""
    func_list += "[]()->Scxml::StateTable{return new $${sn}::$${cn}();},"
}
file_cont= \
    $$inc_list \
    "std::function<Scxml::StateTable *()> creators[] = {" \
    $$func_list \
    "};"
write_file("compiled_tests.h", file_cont)|error("Aborting.")

load(qt_tool)
load(qscxmlcpp)
