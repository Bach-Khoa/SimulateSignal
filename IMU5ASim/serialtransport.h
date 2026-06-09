#pragma once
#include <QObject>
#include <QSerialPort>
#include <QString>

class SerialTransport : public QObject {
    Q_OBJECT
public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport();

    bool open(const QString& portName, int baudRate);
    void close();
    bool isOpen() const;
    bool send(const QByteArray& data);
    QString lastError() const;

private:
    QSerialPort m_port;
    QString     m_lastError;
};
