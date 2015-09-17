/****************************************************************************
 **
 ** Copyright (c) 2015 Digia Plc
 ** For any questions to Digia, please use contact form at http://qt.digia.com/
 **
 ** All Rights Reserved.
 **
 ** NOTICE: All information contained herein is, and remains
 ** the property of Digia Plc and its suppliers,
 ** if any. The intellectual and technical concepts contained
 ** herein are proprietary to Digia Plc
 ** and its suppliers and may be covered by Finnish and Foreign Patents,
 ** patents in process, and are protected by trade secret or copyright law.
 ** Dissemination of this information or reproduction of this material
 ** is strictly forbidden unless prior written permission is obtained
 ** from Digia Plc.
 ****************************************************************************/

#ifndef SCXMLQSTATE_P_H
#define SCXMLQSTATE_P_H

#include <QtScxml/qscxmlqstates.h>

QT_BEGIN_NAMESPACE

class QScxmlState::Data
{
public:
    static Data *get(QScxmlState *s) { return s->d; }

    Data(QScxmlState *state)
        : m_state(state)
        , initInstructions(QScxmlExecutableContent::NoInstruction)
        , onEntryInstructions(QScxmlExecutableContent::NoInstruction)
        , onExitInstructions(QScxmlExecutableContent::NoInstruction)
    {}

    ~Data()
    { qDeleteAll(invokableServiceFactories); }

    QScxmlState *m_state;
    QScxmlExecutableContent::ContainerId initInstructions;
    QScxmlExecutableContent::ContainerId onEntryInstructions;
    QScxmlExecutableContent::ContainerId onExitInstructions;
    QVector<QScxmlInvokableServiceFactory *> invokableServiceFactories;
    QVector<QScxmlInvokableService *> invokedServices;
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
