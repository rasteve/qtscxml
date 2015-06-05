TEMPLATE = subdirs

SUBDIRS += \
    qscxml \
    qscxmlcpp

qscxmlparse.depends = qscxml
qscxmlcpp.depends = qscxml
imports.depends = qscxml

qtHaveModule(qml) {
    SUBDIRS +=  qml-module
}
