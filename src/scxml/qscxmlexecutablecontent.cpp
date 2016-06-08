/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscxmlglobals_p.h"
#include "qscxmlexecutablecontent_p.h"
#include "qscxmlparser_p.h"
#include "qscxmlevent_p.h"

QT_BEGIN_NAMESPACE

using namespace QScxmlExecutableContent;

#ifndef BUILD_QSCXMLC
static int parseTime(const QString &t, bool *ok = 0)
{
    if (t.isEmpty()) {
        if (ok)
            *ok = false;
        return -1;
    }
    bool negative = false;
    int startPos = 0;
    if (t[0] == QLatin1Char('-')) {
        negative = true;
        ++startPos;
    } else if (t[0] == QLatin1Char('+')) {
        ++startPos;
    }
    int pos = startPos;
    for (int endPos = t.length(); pos < endPos; ++pos) {
        auto c = t[pos];
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            break;
    }
    if (pos == startPos) {
        if (ok) *ok = false;
        return -1;
    }
    int value = t.midRef(startPos, pos - startPos).toInt(ok);
    if (ok && !*ok) return -1;
    if (t.length() == pos + 1 && t[pos] == QLatin1Char('s')) {
        value *= 1000;
    } else if (t.length() != pos + 2 || t[pos] != QLatin1Char('m') || t[pos + 1] != QLatin1Char('s')) {
        if (ok) *ok = false;
        return -1;
    }
    return negative ? -value : value;
}

QScxmlExecutionEngine::QScxmlExecutionEngine(QScxmlStateMachine *stateMachine)
    : stateMachine(stateMachine)
{
    Q_ASSERT(stateMachine);
}

bool QScxmlExecutionEngine::execute(ContainerId id, const QVariant &extraData)
{
    Q_ASSERT(stateMachine);

    if (id == NoInstruction)
        return true;

    qint32 *ip = stateMachine->tableData()->instructions() + id;
    this->extraData = extraData;
    bool result = step(ip);
    this->extraData = QVariant();
    return result;
}

bool QScxmlExecutionEngine::step(Instructions &ip)
{
    auto dataModel = stateMachine->dataModel();
    auto tableData = stateMachine->tableData();

    auto instr = reinterpret_cast<Instruction *>(ip);
    switch (instr->instructionType) {
    case Instruction::Sequence: {
        qCDebug(qscxmlLog) << stateMachine << "Executing sequence step";
        InstructionSequence *sequence = reinterpret_cast<InstructionSequence *>(instr);
        ip = sequence->instructions();
        Instructions end = ip + sequence->entryCount;
        while (ip < end) {
            if (!step(ip)) {
                ip = end;
                qCDebug(qscxmlLog) << stateMachine << "Finished sequence step UNsuccessfully";
                return false;
            }
        }
        qCDebug(qscxmlLog) << stateMachine << "Finished sequence step successfully";
        return true;
    }

    case Instruction::Sequences: {
        qCDebug(qscxmlLog) << stateMachine << "Executing sequences step";
        InstructionSequences *sequences = reinterpret_cast<InstructionSequences *>(instr);
        ip += sequences->size();
        for (int i = 0; i != sequences->sequenceCount; ++i) {
            Instructions sequence = sequences->at(i);
            step(sequence);
        }
        qCDebug(qscxmlLog) << stateMachine << "Finished sequences step";
        return true;
    }

    case Instruction::Send: {
        qCDebug(qscxmlLog) << stateMachine << "Executing send step";
        Send *send = reinterpret_cast<Send *>(instr);
        ip += send->size();

        QString delay = tableData->string(send->delay);
        if (send->delayexpr != NoEvaluator) {
            bool ok = false;
            delay = stateMachine->dataModel()->evaluateToString(send->delayexpr, &ok);
            if (!ok)
                return false;
        }

        QScxmlEvent *event = QScxmlEventBuilder(stateMachine, *send).buildEvent();
        if (!event)
            return false;

        if (!delay.isEmpty()) {
            int msecs = parseTime(delay);
            if (msecs >= 0) {
                event->setDelay(msecs);
            } else {
                qCDebug(qscxmlLog) << stateMachine << "failed to parse delay time" << delay;
                return false;
            }
        }

        stateMachine->submitEvent(event);
        return true;
    }

    case Instruction::JavaScript: {
        qCDebug(qscxmlLog) << stateMachine << "Executing script step";
        JavaScript *javascript = reinterpret_cast<JavaScript *>(instr);
        ip += javascript->size();
        bool ok = true;
        dataModel->evaluateToVoid(javascript->go, &ok);
        return ok;
    }

    case Instruction::If: {
        qCDebug(qscxmlLog) << stateMachine << "Executing if step";
        If *_if = reinterpret_cast<If *>(instr);
        ip += _if->size();
        auto blocks = _if->blocks();
        for (qint32 i = 0; i < _if->conditions.count; ++i) {
            bool ok = true;
            if (dataModel->evaluateToBool(_if->conditions.at(i), &ok) && ok) {
                Instructions block = blocks->at(i);
                bool res = step(block);
                qCDebug(qscxmlLog) << stateMachine << "Finished if step";
                return res;
            }
        }

        if (_if->conditions.count < blocks->sequenceCount) {
            Instructions block = blocks->at(_if->conditions.count);
            return step(block);
        }

        return true;
    }

    case Instruction::Foreach: {
        class LoopBody: public QScxmlDataModel::ForeachLoopBody // If only we could put std::function in public API, we could use a lambda here. Alas....
        {
            QScxmlExecutionEngine *engine;
            const Instructions loopStart;

        public:
            LoopBody(QScxmlExecutionEngine *engine, const Instructions loopStart)
                : engine(engine)
                , loopStart(loopStart)
            {}

            bool run() Q_DECL_OVERRIDE
            {
                Instructions ip = loopStart;
                return engine->step(ip);
            }
        };

        qCDebug(qscxmlLog) << stateMachine << "Executing foreach step";
        Foreach *foreach = reinterpret_cast<Foreach *>(instr);
        Instructions loopStart = foreach->blockstart();
        ip += foreach->size();
        bool ok = true;
        LoopBody body(this, loopStart);
        bool evenMoreOk = dataModel->evaluateForeach(foreach->doIt, &ok, &body);
        return ok && evenMoreOk;
    }

    case Instruction::Raise: {
        qCDebug(qscxmlLog) << stateMachine << "Executing raise step";
        Raise *raise = reinterpret_cast<Raise *>(instr);
        ip += raise->size();
        auto name = tableData->string(raise->event);
        auto event = new QScxmlEvent;
        event->setName(name);
        event->setEventType(QScxmlEvent::InternalEvent);
        stateMachine->submitEvent(event);
        return true;
    }

    case Instruction::Log: {
        qCDebug(qscxmlLog) << stateMachine << "Executing log step";
        Log *log = reinterpret_cast<Log *>(instr);
        ip += log->size();
        bool ok = true;
        QString str = dataModel->evaluateToString(log->expr, &ok);
        if (ok) {
            const QString label = tableData->string(log->label);
            qCDebug(scxmlLog) << label << ":" << str;
            QMetaObject::invokeMethod(stateMachine,
                                      "log",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, label),
                                      Q_ARG(QString, str));
        }
        return ok;
    }

    case Instruction::Cancel: {
        qCDebug(qscxmlLog) << stateMachine << "Executing cancel step";
        Cancel *cancel = reinterpret_cast<Cancel *>(instr);
        ip += cancel->size();
        QString e = tableData->string(cancel->sendid);
        bool ok = true;
        if (cancel->sendidexpr != NoEvaluator)
            e = dataModel->evaluateToString(cancel->sendidexpr, &ok);
        if (ok && !e.isEmpty())
            stateMachine->cancelDelayedEvent(e);
        return ok;
    }

    case Instruction::Assign: {
        qCDebug(qscxmlLog) << stateMachine << "Executing assign step";
        Assign *assign = reinterpret_cast<Assign *>(instr);
        ip += assign->size();
        bool ok = true;
        dataModel->evaluateAssignment(assign->expression, &ok);
        return ok;
    }

    case Instruction::Initialize: {
        qCDebug(qscxmlLog) << stateMachine << "Executing initialize step";
        Initialize *init = reinterpret_cast<Initialize *>(instr);
        ip += init->size();
        bool ok = true;
        dataModel->evaluateInitialization(init->expression, &ok);
        return ok;
    }

    case Instruction::DoneData: {
        qCDebug(qscxmlLog) << stateMachine << "Executing DoneData step";
        DoneData *doneData = reinterpret_cast<DoneData *>(instr);

        QString eventName = QStringLiteral("done.state.") + extraData.toString();
        QScxmlEventBuilder event(stateMachine, eventName, doneData);
        auto e = event();
        e->setEventType(QScxmlEvent::InternalEvent);
        qCDebug(qscxmlLog) << stateMachine << "submitting event" << eventName;
        stateMachine->submitEvent(e);
        return true;
    }

    default:
        Q_UNREACHABLE();
        return false;
    }
}
#endif // BUILD_QSCXMLC

QT_END_NAMESPACE
