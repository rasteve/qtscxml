CONFIG += tests_need_tools examples_need_tools

FEATURES += mkspecs/features/qscxmlc.prf

features.files = $$FEATURES
features.path = $$[QT_HOST_DATA]/mkspecs/features/

INSTALLS += features

OTHER_FILES += \
    $$FEATURES \
    .qmake.conf \
    sync.profile \
    .gitignore \
    TODO.txt

load(qt_parts)
