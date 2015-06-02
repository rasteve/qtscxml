TEMPLATE = subdirs

SUBDIRS += \
    qscxmllib \
    qscxmlcpp

qscxmlparse.depends = qscxmllib
qscxmlcpp.depends = qscxmllib
imports.depends = qscxmllib

qtHaveModule(qml) {
    SUBDIRS +=  qml-module
}
