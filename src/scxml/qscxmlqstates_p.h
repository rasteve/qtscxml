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

#ifndef SCXMLQSTATE_P_H
#define SCXMLQSTATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtScxml/qscxmlqstates.h>

QT_BEGIN_NAMESPACE

class QScxmlStatePrivate
{
public:
    static QScxmlStatePrivate *get(QScxmlState *s) { return s->d; }

    QScxmlStatePrivate(QScxmlState *state)
        : m_state(state)
        , initInstructions(QScxmlExecutableContent::NoInstruction)
        , onEntryInstructions(QScxmlExecutableContent::NoInstruction)
        , onExitInstructions(QScxmlExecutableContent::NoInstruction)
    {}

    ~QScxmlStatePrivate()
    { qDeleteAll(invokableServiceFactories); }

    QScxmlState *m_state;
    QScxmlExecutableContent::ContainerId initInstructions;
    QScxmlExecutableContent::ContainerId onEntryInstructions;
    QScxmlExecutableContent::ContainerId onExitInstructions;
    QVector<QScxmlInvokableServiceFactory *> invokableServiceFactories;
    QVector<QScxmlInvokableService *> invokedServices;
    QVector<QScxmlInvokableService *> servicesWaitingToStart;
};

class QScxmlFinalState::Data
{
public:
    static Data *get(QScxmlFinalState *s) { return s->d; }

    Data()
        : doneData(QScxmlExecutableContent::NoInstruction)
        , onEntryInstructions(QScxmlExecutableContent::NoInstruction)
        , onExitInstructions(QScxmlExecutableContent::NoInstruction)
    {}

    QScxmlExecutableContent::ContainerId doneData;
    QScxmlExecutableContent::ContainerId onEntryInstructions;
    QScxmlExecutableContent::ContainerId onExitInstructions;
};

QT_END_NAMESPACE

#endif // SCXMLQSTATE_P_H
