#include "SerialConnection.h"

SerialConnection::SerialConnection(QObject* parent)
    : AbstractConnection(parent)
{
    m_port.setBaudRate(QSerialPort::Baud9600);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    connect(&m_port, &QSerialPort::readyRead,
            this, &SerialConnection::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred,
            this, &SerialConnection::onErrorOccurred);
}

void SerialConnection::setPortName(const QString& port)
{
    m_port.setPortName(port);
}

QString SerialConnection::portName() const
{
    return m_port.portName();
}

bool SerialConnection::open()
{
    if (m_port.portName().isEmpty())
        return false;

    if (!m_port.open(QIODevice::ReadWrite))
        return false;

    emit connectionOpened();
    return true;
}

void SerialConnection::close()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit connectionClosed();
    }
}

bool SerialConnection::isOpen() const
{
    return m_port.isOpen();
}

QString SerialConnection::description() const
{
    return QString("%1 | 9600 8N1").arg(m_port.portName());
}

bool SerialConnection::write(const QByteArray& data)
{
    if (!m_port.isOpen())
        return false;
    return m_port.write(data) == data.size();
}

void SerialConnection::setDtr(bool asserted)
{
    if (m_port.isOpen())
        m_port.setDataTerminalReady(asserted);
}

void SerialConnection::onReadyRead()
{
    const QByteArray data = m_port.readAll();
    if (!data.isEmpty())
        emit dataReceived(data);
}

void SerialConnection::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    const QString msg = m_port.errorString();
    emit errorOccurred(msg);

    if (error != QSerialPort::TimeoutError && m_port.isOpen()) {
        m_port.close();
        emit connectionClosed();
    }
}
