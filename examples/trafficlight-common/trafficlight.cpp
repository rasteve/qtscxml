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

#include "trafficlight.h"

#include <QtWidgets>

class TrafficLightWidget : public QWidget
{
public:
    TrafficLightWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        m_red = new LightWidget(Qt::red);
        vbox->addWidget(m_red);
        m_yellow = new LightWidget(Qt::yellow);
        vbox->addWidget(m_yellow);
        m_green = new LightWidget(Qt::green);
        vbox->addWidget(m_green);
        QPalette pal = palette();
        pal.setColor(QPalette::Background, Qt::black);
        setPalette(pal);
        setAutoFillBackground(true);
    }

    LightWidget *redLight() const
    { return m_red; }
    LightWidget *yellowLight() const
    { return m_yellow; }
    LightWidget *greenLight() const
    { return m_green; }

private:
    LightWidget *m_red;
    LightWidget *m_yellow;
    LightWidget *m_green;
};

TrafficLight::TrafficLight(Scxml::StateMachine *machine, QWidget *parent)
    : QWidget(parent)
    , m_machine(machine)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    TrafficLightWidget *widget = new TrafficLightWidget();
    vbox->addWidget(widget);
    vbox->setMargin(0);

    QPushButton *button = new QPushButton;
    vbox->addWidget(button);
    button->setCheckable(true);
    button->setText(QStringLiteral("Pause"));
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleWorking(bool)));

    Q_ASSERT(machine->init());

    machine->connect(QStringLiteral("red"), SIGNAL(activeChanged(bool)),
                     widget->redLight(), SLOT(switchLight(bool)));
    machine->connect(QStringLiteral("redGoingGreen"), SIGNAL(activeChanged(bool)),
                     widget->redLight(), SLOT(switchLight(bool)));
    machine->connect(QStringLiteral("yellow"), SIGNAL(activeChanged(bool)),
                     widget->yellowLight(), SLOT(switchLight(bool)));
    machine->connect(QStringLiteral("blinking"), SIGNAL(activeChanged(bool)),
                     widget->yellowLight(), SLOT(switchLight(bool)));
    machine->connect(QStringLiteral("green"), SIGNAL(activeChanged(bool)),
                     widget->greenLight(), SLOT(switchLight(bool)));
}

void TrafficLight::toggleWorking(bool pause)
{
    m_machine->submitEvent(pause ? "smash" : "repair");
}

LightWidget::LightWidget(const QColor &color, QWidget *parent)
    : QWidget(parent)
    , m_color(color)
    , m_on(false)
{}

bool LightWidget::isOn() const
{ return m_on; }

void LightWidget::setOn(bool on)
{
    if (on == m_on)
        return;
    m_on = on;
    update();
}

void LightWidget::switchLight(bool onoff)
{ setOn(onoff); }

void LightWidget::paintEvent(QPaintEvent *)
{
    if (!m_on)
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(m_color);
    painter.drawEllipse(0, 0, width(), height());
}
