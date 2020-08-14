TEMPLATE = subdirs
QT_FOR_CONFIG += qml

METATYPETESTS += \
    qqmlmetatype

PUBLICTESTS += \
    qqmlstatemachine

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
