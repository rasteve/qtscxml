TEMPLATE      = subdirs

SUBDIRS       = pingpong

qtHaveModule(widgets) {
    SUBDIRS +=  rogue \
                trafficlight
}
