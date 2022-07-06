# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(TARGET Qt::StateMachineQml)
    add_subdirectory(statemachine)
endif()
if(TARGET Qt::ScxmlQml)
    add_subdirectory(scxml)
endif()
