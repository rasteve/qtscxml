TEMPLATE = subdirs
QT_FOR_CONFIG += qml

METATYPETESTS += \
    qqmlstatemachinemetatype

PUBLICTESTS += \
    qqmlstatemachine

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
