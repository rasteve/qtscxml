# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(factorial)
qt_internal_add_example(pingpong)
if(TARGET Qt::Widgets)
    qt_internal_add_example(eventtransitions)
    qt_internal_add_example(rogue)
    qt_internal_add_example(trafficlight)
    qt_internal_add_example(twowaybutton)
endif()
