#include "serialtransport.h"
#include <QMutexLocker>
#include <QDebug>

SerialTransport::SerialTransport(QObject* parent) : QObject(parent) {}
SerialTransport::~SerialTransport() { close(); }

// =============================================================================
// Windows implementation — uses CreateFile with FILE_SHARE_READ|FILE_SHARE_WRITE
// so serial monitor tools (Serial Port Utility, etc.) can concurrently open the
// same COM port for reading without getting ACCESS DENIED.
// =============================================================================
#ifdef Q_OS_WIN

bool SerialTransport::open(const QString& portName, int baudRate) {
    QMutexLocker lk(&m_mutex);
    if (m_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
    }

    // Prefix with \\.\  to support COM10 and above
    const QString devPath = "\\\\.\\" + portName;

    m_hPort = CreateFileW(
        reinterpret_cast<LPCWSTR>(devPath.utf16()),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,   // allow monitor tools to co-open
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (m_hPort == INVALID_HANDLE_VALUE) {
        m_lastError = QString("Cannot open %1 (Windows error %2)")
                          .arg(portName).arg(static_cast<uint>(GetLastError()));
        return false;
    }

    if (!applyCommState(baudRate)) {
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
        return false;
    }

    // Purge any stale data
    PurgeComm(m_hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);
    m_bytesSent = 0;
    qDebug() << "[SerialTransport] Opened" << portName << "@" << baudRate;
    return true;
}

bool SerialTransport::applyCommState(int baudRate) {
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hPort, &dcb)) {
        m_lastError = QString("GetCommState failed (error %1)").arg(static_cast<uint>(GetLastError()));
        return false;
    }

    dcb.BaudRate        = static_cast<DWORD>(baudRate);
    dcb.ByteSize        = 8;
    dcb.StopBits        = ONESTOPBIT;
    dcb.Parity          = NOPARITY;
    dcb.fParity         = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDtrControl     = DTR_CONTROL_DISABLE;
    dcb.fRtsControl     = RTS_CONTROL_DISABLE;
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;
    dcb.fAbortOnError   = FALSE;

    if (!SetCommState(m_hPort, &dcb)) {
        m_lastError = QString("SetCommState failed (error %1)").arg(static_cast<uint>(GetLastError()));
        return false;
    }

    // Timeouts: non-blocking write (return immediately after bytes queued to driver)
    COMMTIMEOUTS to = {};
    to.WriteTotalTimeoutConstant    = 50;   // ms total write timeout
    to.WriteTotalTimeoutMultiplier  = 0;
    SetCommTimeouts(m_hPort, &to);
    return true;
}

void SerialTransport::close() {
    QMutexLocker lk(&m_mutex);
    if (m_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
    }
}

bool SerialTransport::isOpen() const {
    return m_hPort != INVALID_HANDLE_VALUE;
}

bool SerialTransport::send(const QByteArray& data) {
    QMutexLocker lk(&m_mutex);
    if (m_hPort == INVALID_HANDLE_VALUE) {
        m_lastError = "Port not open";
        return false;
    }

    DWORD written = 0;
    if (!WriteFile(m_hPort, data.constData(), static_cast<DWORD>(data.size()), &written, nullptr)) {
        DWORD err = GetLastError();
        m_lastError = QString("WriteFile failed (WinError %1)").arg(err);
        qDebug() << "[SerialTransport] SEND FAIL:" << m_lastError;
        return false;
    }
    if (static_cast<int>(written) != data.size()) {
        m_lastError = QString("Partial write: %1/%2 bytes").arg(written).arg(data.size());
        qDebug() << "[SerialTransport] PARTIAL:" << m_lastError;
        return false;
    }
    m_bytesSent += written;
    return true;
}

QString SerialTransport::lastError() const { return m_lastError; }

// =============================================================================
// Non-Windows fallback — original QSerialPort implementation
// =============================================================================
#else

bool SerialTransport::open(const QString& portName, int baudRate) {
    QMutexLocker lk(&m_mutex);
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
    QMutexLocker lk(&m_mutex);
    if (m_port.isOpen()) m_port.close();
}

bool SerialTransport::isOpen() const { return m_port.isOpen(); }

bool SerialTransport::send(const QByteArray& data) {
    QMutexLocker lk(&m_mutex);
    if (!m_port.isOpen()) { m_lastError = "Port not open"; return false; }
    qint64 written = m_port.write(data);
    if (written != data.size()) { m_lastError = m_port.errorString(); return false; }
    m_port.flush();
    return true;
}

QString SerialTransport::lastError() const { return m_lastError; }

#endif
