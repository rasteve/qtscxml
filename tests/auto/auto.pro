TEMPLATE = subdirs
SUBDIRS = cmake\
          compiled\
          dynamicmetaobject\
          parser\
          scion\
          statemachine \
          statemachineinfo \
          qml \
          qmltest

!uikit: SUBDIRS += qstatemachine
