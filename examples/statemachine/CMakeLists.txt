# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_subdirectory(statemachine)

if(TARGET Qt::Widgets)
    add_subdirectory(graphicsview)
    if(QT_FEATURE_animation)
        add_subdirectory(animation)
    endif()
endif()
