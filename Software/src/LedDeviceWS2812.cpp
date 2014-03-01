/*
 * LedDeviceWS2812.cpp
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

#include "LedDeviceWS2812.hpp"
#include "PrismatikMath.hpp"
#include "Settings.hpp"
#include "debug.h"
#include "stdio.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QObject>

using namespace SettingsScope;


void SenderThread::run()
{
    while(true)
    {
        if (isInterruptionRequested())
            break;
    }
}

LedDeviceWS2812::LedDeviceWS2812(/*const QString &portName, const int baudRate,*/ QObject *parent) : AbstractLedDevice(parent)
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;

    //m_portName = portName;
    //m_baudRate = baudRate;

//    m_gamma = Settings::getDeviceGamma();
//    m_brightness = Settings::getDeviceBrightness();
//    m_colorSequence =Settings::getColorSequence(SupportedDevices::DeviceTypeAdalight);
    m_port = NULL;

    // TODO: think about init m_savedColors in all ILedDevices

    DEBUG_LOW_LEVEL << Q_FUNC_INFO << "initialized";

    SenderThread thread;
    thread.start();
    thread.requestInterruption();
}

LedDeviceWS2812::~LedDeviceWS2812()
{
    close();
}

void LedDeviceWS2812::close()
{
    if (m_port != NULL) {
        m_port->close();

        delete m_port;
        m_port = NULL;
    }
}

void LedDeviceWS2812::setColors(const QList<QRgb> & colors)
{
    // Save colors for showing changes of the brightness
    m_colorsSaved = colors;

    resizeColorsBuffer(colors.count());

    applyColorModifications(colors, m_colorsBuffer);


    m_writeBuffer.clear();
    m_writeBuffer.append(m_writeBufferHeader);

    for (int i = 0; i < m_colorsBuffer.count(); i++)
    {
        StructRgb color = m_colorsBuffer[i];

        color.r = color.r >> 4;
        color.g = color.g >> 4;
        color.b = color.b >> 4;

        if (m_colorSequence == "RBG")
        {
            m_writeBuffer.append(color.r);
            m_writeBuffer.append(color.b);
            m_writeBuffer.append(color.g);
        }
        else if (m_colorSequence == "BRG")
        {
            m_writeBuffer.append(color.b);
            m_writeBuffer.append(color.r);
            m_writeBuffer.append(color.g);
        }
        else if (m_colorSequence == "BGR")
        {
            m_writeBuffer.append(color.b);
            m_writeBuffer.append(color.g);
            m_writeBuffer.append(color.r);
        }
        else if (m_colorSequence == "GRB")
        {
            m_writeBuffer.append(color.g);
            m_writeBuffer.append(color.r);
            m_writeBuffer.append(color.b);
        }
        else if (m_colorSequence == "GBR")
        {
            m_writeBuffer.append(color.g);
            m_writeBuffer.append(color.b);
            m_writeBuffer.append(color.r);
        }
        else
        {
            m_writeBuffer.append(color.r);
            m_writeBuffer.append(color.g);
            m_writeBuffer.append(color.b);
        }
    }

    bool ok = writeBuffer(m_writeBuffer);

    emit commandCompleted(ok);
}

void LedDeviceWS2812::switchOffLeds()
{
    int count = m_colorsSaved.count();
    m_colorsSaved.clear();

    for (int i = 0; i < count; i++)
        m_colorsSaved << 0;

    m_writeBuffer.clear();
    m_writeBuffer.append(m_writeBufferHeader);

    for (int i = 0; i < count; i++) {
        m_writeBuffer.append((char)0)
                     .append((char)0)
                     .append((char)0);
    }

    bool ok = writeBuffer(m_writeBuffer);
    emit commandCompleted(ok);
}

void LedDeviceWS2812::setRefreshDelay(int /*value*/)
{
    emit commandCompleted(true);
}

void LedDeviceWS2812::setColorDepth(int /*value*/)
{
    emit commandCompleted(true);
}

void LedDeviceWS2812::setSmoothSlowdown(int /*value*/)
{
    emit commandCompleted(true);
}

void LedDeviceWS2812::setColorSequence(QString value)
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << value;

    m_colorSequence = value;
    setColors(m_colorsSaved);
}

void LedDeviceWS2812::requestFirmwareVersion()
{
    emit firmwareVersion("unknown (ws2812 device)");
    emit commandCompleted(true);
}

void LedDeviceWS2812::updateDeviceSettings()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO;

    AbstractLedDevice::updateDeviceSettings();
    setColorSequence(Settings::getColorSequence(SupportedDevices::DeviceTypeWS2812));
}

void LedDeviceWS2812::open()
{
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << sender();

//    m_gamma = Settings::getDeviceGamma();
//    m_brightness = Settings::getDeviceBrightness();

    if (m_port != NULL)
        m_port->close();
    else
        m_port = new QSerialPort();

    m_port->setPortName(m_portName);// Settings::getAdalightSerialPortName());

    m_port->open(QIODevice::WriteOnly);
    bool ok = m_port->isOpen();

    // Ubuntu 10.04: on every second attempt to open the device leads to failure
    if (ok == false)
    {
        // Try one more time
        m_port->open(QIODevice::WriteOnly);
        ok = m_port->isOpen();
    }

    if (ok)
    {
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Serial device" << m_port->portName() << "open";

        ok = m_port->setBaudRate(m_baudRate);//Settings::getAdalightSerialPortBaudRate());
        if (ok)
        {
            ok = m_port->setDataBits(QSerialPort::Data8);
            if (ok)
            {
                DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Baud rate  :" << m_port->baudRate();
                DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Data bits  :" << m_port->dataBits();
                DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Parity     :" << m_port->parity();
                DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Stop bits  :" << m_port->stopBits();
                DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Flow       :" << m_port->flowControl();
            } else {
                qWarning() << Q_FUNC_INFO << "Set data bits 8 fail";
            }
        } else {
            qWarning() << Q_FUNC_INFO << "Set baud rate" << m_baudRate << "fail";
        }

    } else {
        qWarning() << Q_FUNC_INFO << "Serial device" << m_port->portName() << "open fail. " << m_port->errorString();
        DEBUG_OUT << Q_FUNC_INFO << "Available ports:";
        QList<QSerialPortInfo> availPorts = QSerialPortInfo::availablePorts();
        for(int i=0; i < availPorts.size(); i++) {
            DEBUG_OUT << Q_FUNC_INFO << availPorts[i].portName();
        }
    }

    emit openDeviceSuccess(ok);
}

bool LedDeviceWS2812::writeBuffer(const QByteArray & buff)
{
    DEBUG_MID_LEVEL << Q_FUNC_INFO << "Hex:" << buff.toHex();

    if (m_port == NULL || m_port->isOpen() == false)
        return false;

    int bytesWritten = m_port->write(buff);

    if (bytesWritten != buff.count())
    {
        qWarning() << Q_FUNC_INFO << "bytesWritten != buff.count():" << bytesWritten << buff.count() << " " << m_port->errorString();
        return false;
    }

    return true;
}

void LedDeviceWS2812::resizeColorsBuffer(int buffSize)
{
    if (m_colorsBuffer.count() == buffSize)
        return;

    m_colorsBuffer.clear();

    if (buffSize > MaximumNumberOfLeds::Adalight)
    {
        qCritical() << Q_FUNC_INFO << "buffSize > MaximumNumberOfLeds::Adalight" << buffSize << ">" << MaximumNumberOfLeds::Adalight;

        buffSize = MaximumNumberOfLeds::Adalight;
    }

    for (int i = 0; i < buffSize; i++)
    {
        m_colorsBuffer << StructRgb();
    }

    reinitBufferHeader(buffSize);
}

void LedDeviceWS2812::reinitBufferHeader(int ledsCount)
{
    m_writeBufferHeader.clear();

    // Initialize buffer header
    int ledsCountHi = ((ledsCount - 1) >> 8) & 0xff;
    int ledsCountLo = (ledsCount  - 1) & 0xff;

    m_writeBufferHeader.append((char)'A');
    m_writeBufferHeader.append((char)'d');
    m_writeBufferHeader.append((char)'a');
    m_writeBufferHeader.append((char)ledsCountHi);
    m_writeBufferHeader.append((char)ledsCountLo);
    m_writeBufferHeader.append((char)(ledsCountHi ^ ledsCountLo ^ 0x55));
}
