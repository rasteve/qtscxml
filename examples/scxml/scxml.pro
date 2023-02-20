TEMPLATE = subdirs

qtHaveModule(widgets) {
    SUBDIRS +=  trafficlight-widgets-static
    SUBDIRS +=  trafficlight-widgets-dynamic
    SUBDIRS +=  calculator-widgets
    SUBDIRS +=  sudoku
}

qtHaveModule(quick) {
    SUBDIRS +=  calculator-qml
    SUBDIRS +=  trafficlight-qml-static
    SUBDIRS +=  trafficlight-qml-dynamic
    SUBDIRS +=  trafficlight-qml-simple
    SUBDIRS +=  mediaplayer
    SUBDIRS +=  invoke-static
    SUBDIRS +=  invoke-dynamic
}

SUBDIRS += ftpclient
