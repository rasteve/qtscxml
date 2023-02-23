# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(statemachine)

if(TARGET Qt::Widgets)
    if(QT_FEATURE_animation)
        add_subdirectory(animation)
    endif()
endif()
