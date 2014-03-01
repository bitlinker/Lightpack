/*
 * LedDeviceWS2812.hpp
 *
 *  Created on: 08.01.2014
 *      Author: Ivan Samoylenko
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "AbstractLedDevice.hpp"
#include "colorspace_types.h"
#include <QtSerialPort/QSerialPort>


class SenderThread : public QThread
{
    Q_OBJECT

public:
    //! Constructor
    SenderThread() {};

    //! Destructor
    virtual ~SenderThread() {};

protected:
    //! Run method
    void run();

private:

};


class LedDeviceWS2812 : public AbstractLedDevice
{
    Q_OBJECT
public:
    LedDeviceWS2812(/*const QString &portName, const int baudRate, */QObject * parent = 0);
    virtual ~LedDeviceWS2812();

public slots:
    const QString name() const { return "ws2812"; }
    void open();
    void close();
    void setColors(const QList<QRgb> & /*colors*/);
    void switchOffLeds();
    void setRefreshDelay(int /*value*/);
    void setColorDepth(int /*value*/);
    void setSmoothSlowdown(int /*value*/);
    void setColorSequence(QString value);
    void requestFirmwareVersion();
    void updateDeviceSettings();
    size_t maxLedsCount() { return 255;}
    size_t defaultLedsCount() {return 1;}

private:
    bool writeBuffer(const QByteArray & buff);
    void resizeColorsBuffer(int buffSize);
    void reinitBufferHeader(int ledsCount);

private:

    QSerialPort *m_port;
    QByteArray m_writeBufferHeader;
    QByteArray m_writeBuffer;
    QString m_portName;
    int m_baudRate;
};
