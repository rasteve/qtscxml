requires(qtHaveModule(widgets))

TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = statemachine

qtConfig(animation): SUBDIRS += animation
