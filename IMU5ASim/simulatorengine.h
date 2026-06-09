#pragma once
#include <QThread>
#include <QAtomicInt>
#include <QVector>
#include <QByteArray>
#include "imuframe.h"
#include "scaleconfig.h"
#include "packetbuilder.h"
#include "serialtransport.h"

class SimulatorEngine : public QThread {
    Q_OBJECT
public:
    explicit SimulatorEngine(QObject* parent = nullptr);

    void setup(const QVector<ImuFrame>& frames,
               const ScaleConfig& scale,
               SerialTransport* transport,
               bool loop);

    void requestStop();

signals:
    // Emitted every ~100ms (every 12–13 frames) to avoid overwhelming the UI.
    void statsUpdated(int index, int total, qint64 sentTotal);
    // Emitted for each new packet (throttled to ~5Hz for hex log).
    void packetReady(QByteArray packet, ImuFrame frame);
    void errorOccurred(QString message);
    void finished();

protected:
    void run() override;

private:
    QVector<ImuFrame> m_frames;
    ScaleConfig       m_scale;
    SerialTransport*  m_transport = nullptr;
    bool              m_loop      = true;
    QAtomicInt        m_stop{0};
};
