# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_test(tst_scxmlcoutput_default
    SOURCES
        tst_scxmlcoutput.cpp
    PUBLIC_LIBRARIES
        Qt::Scxml
)

# This testcase tests that the default code generation,
# ie. no options provided for the qt6_add_statecharts
qt6_add_statecharts(tst_scxmlcoutput_default
    ../shared/ids1.scxml
)
