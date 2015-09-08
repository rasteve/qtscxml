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

#include "scxmlqstates.h"

QT_BEGIN_NAMESPACE

namespace Scxml {

class ScxmlState::Data
{
public:
    static Data *get(ScxmlState *s) { return s->d; }

    Data(ScxmlState *state)
        : m_state(state)
        , initInstructions(ExecutableContent::NoInstruction)
        , onEntryInstructions(ExecutableContent::NoInstruction)
        , onExitInstructions(ExecutableContent::NoInstruction)
    {}

    ~Data()
    { qDeleteAll(invokableServiceFactories); }

    ScxmlState *m_state;
    ExecutableContent::ContainerId initInstructions;
    ExecutableContent::ContainerId onEntryInstructions;
    ExecutableContent::ContainerId onExitInstructions;
    QVector<ScxmlInvokableServiceFactory *> invokableServiceFactories;
    QVector<ScxmlInvokableService *> invokedServices;
};

class ScxmlFinalState::Data
{
public:
    static Data *get(ScxmlFinalState *s) { return s->d; }

    Data()
        : doneData(ExecutableContent::NoInstruction)
        , onEntryInstructions(ExecutableContent::NoInstruction)
        , onExitInstructions(ExecutableContent::NoInstruction)
    {}

    ExecutableContent::ContainerId doneData;
    ExecutableContent::ContainerId onEntryInstructions;
    ExecutableContent::ContainerId onExitInstructions;
};

} // Scxml namespace

QT_END_NAMESPACE

#endif // SCXMLQSTATE_P_H
