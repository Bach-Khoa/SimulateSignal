#include "serialtransport.h"

SerialTransport::SerialTransport(QObject* parent) : QObject(parent) {}

SerialTransport::~SerialTransport() { close(); }

bool SerialTransport::open(const QString& portName, int baudRate) {
    if (m_port.isOpen()) m_port.close();

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::WriteOnly)) {
        m_lastError = m_port.errorString();
        return false;
    }
    m_port.clear();
    return true;
}

void SerialTransport::close() {
    if (m_port.isOpen()) m_port.close();
}

bool SerialTransport::isOpen() const { return m_port.isOpen(); }

bool SerialTransport::send(const QByteArray& data) {
    if (!m_port.isOpen()) {
        m_lastError = "Port not open";
        return false;
    }
    qint64 written = m_port.write(data);
    if (written != data.size()) {
        m_lastError = m_port.errorString();
        return false;
    }
    return true;
}

QString SerialTransport::lastError() const { return m_lastError; }
