/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScxml module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#ifndef EXECUTABLECONTENT_H
#define EXECUTABLECONTENT_H

#include <QtScxml/qscxmlglobals.h>

#include <QVector>

QT_BEGIN_NAMESPACE

namespace QScxmlExecutableContent {

typedef int ContainerId;
enum { NoInstruction = -1 };
typedef qint32 StringId;
typedef QVector<StringId> StringIds;
enum { NoString = -1 };
typedef qint32 ByteArrayId;
enum { NoByteArray = -1 };
typedef qint32 *Instructions;

} // ExecutableContent namespace

QT_END_NAMESPACE

#endif // EXECUTABLECONTENT_H
