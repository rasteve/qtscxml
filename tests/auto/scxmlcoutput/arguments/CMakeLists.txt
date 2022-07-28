# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_test(tst_scxmlcoutput_arguments
    SOURCES
        tst_scxmlcoutput.cpp
    LIBRARIES
        Qt::Scxml
)

# This testcase tests providing the namespace
qt6_add_statecharts(tst_scxmlcoutput_arguments
    ../shared/ids1.scxml
    OPTIONS --classname MyClass
)
