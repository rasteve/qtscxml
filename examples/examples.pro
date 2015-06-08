TEMPLATE = subdirs

qtHaveModule(widgets) {
    SUBDIRS +=  trafficlight-widgets-static
    SUBDIRS +=  trafficlight-widgets-dynamic
}

qtHaveModule(qml) {
    SUBDIRS +=  trafficlight-qml-static
    SUBDIRS +=  trafficlight-qml-dynamic
}
