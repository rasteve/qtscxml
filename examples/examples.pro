TEMPLATE = subdirs

qtHaveModule(widgets) {
    SUBDIRS +=  trafficlight-widgets
}

qtHaveModule(qml) {
    SUBDIRS +=  trafficlight-qml
}
