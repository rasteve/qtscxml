TEMPLATE = subdirs

SUBDIRS += \
    qscxmllib \
    qscxmlparse \
    qscxmlcpp

qscxmlparse.depends = qscxmllib
qscxmlcpp.depends = qscxmllib
imports.depends = qscxmllib
