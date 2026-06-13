#pragma once
#include <QObject>
#include <QString>
#include <QMutex>
#include <QByteArray>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <QSerialPort>
#endif

class SerialTransport : public QObject {
    Q_OBJECT
public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport();

    bool    open(const QString& portName, int baudRate);
    void    close();
    bool    isOpen() const;
    bool    send(const QByteArray& data);
    QString lastError() const;
    qint64  bytesSent() const { return m_bytesSent; }

private:
#ifdef Q_OS_WIN
    bool applyCommState(int baudRate);

    HANDLE  m_hPort = INVALID_HANDLE_VALUE;
#else
    QSerialPort m_port;
#endif

    QString m_lastError;
    QMutex  m_mutex;
    qint64  m_bytesSent = 0;
};
