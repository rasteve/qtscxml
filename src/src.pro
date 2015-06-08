TEMPLATE = subdirs

SUBDIRS += qscxml
qscxmlparse.depends = qscxml

qtHaveModule(qml) {
    SUBDIRS += imports
    imports.depends = qscxml
}
