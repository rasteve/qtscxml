TEMPLATE = subdirs

SUBDIRS += \
    qscxmllib \
    qscxmlparse \
    qscxmlserver \
    qscxmlcpp

qscxmlparse.depends = qscxmllib
qscxmlserver.depends = qscxmllib
qscxmlcpp.depends = qscxmllib
imports.depends = qscxmllib
