load(qt_parts)
FEATURES += \
    mkspecs/features/qscxmlcpp.prf

features.files = $$FEATURES
features.path = $$[QT_HOST_DATA]/mkspecs/features/

INSTALLS += features

OTHER_FILES += \
    $$FEATURES \
    .qmake.conf \
    sync.profile \
    .gitignore \
    TODO.txt
