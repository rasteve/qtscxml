/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
#include "pinball.h"

QT_USE_NAMESPACE

MainWindow::MainWindow(Pinball *machine, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    m_machine(machine)
{
    ui->setupUi(this);

    // lights
    initAndConnect(QLatin1String("cLightOn"), ui->cLabel);
    initAndConnect(QLatin1String("rLightOn"), ui->rLabel);
    initAndConnect(QLatin1String("aLightOn"), ui->aLabel);
    initAndConnect(QLatin1String("zLightOn"), ui->zLabel);
    initAndConnect(QLatin1String("yLightOn"), ui->yLabel);
    initAndConnect(QLatin1String("hurryLightOn"), ui->hurryLabel);
    initAndConnect(QLatin1String("jackpotLightOn"), ui->jackpotLabel);
    initAndConnect(QLatin1String("gameOverLightOn"), ui->gameOverLabel);

    // help labels
    initAndConnect(QLatin1String("offState"), ui->offStateLabel);
    initAndConnect(QLatin1String("normalState"), ui->normalStateLabel);
    initAndConnect(QLatin1String("hurryState"), ui->hurryStateLabel);
    initAndConnect(QLatin1String("jackpotStateOn"), ui->jackpotStateLabel);

    // context enablement
    initAndConnect(QLatin1String("offState"), ui->startButton);
    initAndConnect(QLatin1String("onState"), ui->cButton);
    initAndConnect(QLatin1String("onState"), ui->rButton);
    initAndConnect(QLatin1String("onState"), ui->aButton);
    initAndConnect(QLatin1String("onState"), ui->zButton);
    initAndConnect(QLatin1String("onState"), ui->yButton);
    initAndConnect(QLatin1String("onState"), ui->ballOutButton);

    // gui interaction
    connect(ui->cButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("letterTriggered.C");
            });
    connect(ui->rButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("letterTriggered.R");
            });
    connect(ui->aButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("letterTriggered.A");
            });
    connect(ui->zButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("letterTriggered.Z");
            });
    connect(ui->yButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("letterTriggered.Y");
            });
    connect(ui->startButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("startTriggered");
            });
    connect(ui->ballOutButton, &QAbstractButton::clicked,
            [this] { m_machine->submitEvent("ballOutTriggered");
            });

    // datamodel update
    connect(m_machine, &Pinball::event_updateScore,
            this, &MainWindow::updateScore);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initAndConnect(const QString &state, QWidget *widget)
{
    widget->setEnabled(m_machine->isActive(state));
    m_machine->connect(state, SIGNAL(activeChanged(bool)),
                       widget, SLOT(setEnabled(bool)));
}

void MainWindow::updateScore(const QVariant &data)
{
    const QString highScore = data.toMap().value("highScore").toString();
    ui->highScoreLabel->setText(highScore);
    const QString score = data.toMap().value("score").toString();
    ui->scoreLabel->setText(score);
    qDebug() << "HERE!!!" << highScore << score;
}

