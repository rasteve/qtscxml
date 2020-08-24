TEMPLATE = subdirs

SUBDIRS += scxml statemachine

qtHaveModule(qml) {
    SUBDIRS += imports
    imports.depends = scxml statemachine
}
