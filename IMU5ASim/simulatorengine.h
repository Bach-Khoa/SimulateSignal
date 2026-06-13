#pragma once
#include <QThread>
#include <QAtomicInt>
#include <QMutex>
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

    // CSV mode: cycles through frames at 125 Hz
    void setup(const QVector<ImuFrame>& frames,
               const ScaleConfig& scale,
               SerialTransport* transport,
               bool loop);

    // Manual mode: transmits a single live frame at 125 Hz
    // loop=true → continuous; loop=false → send 1 packet then stop
    void setupManual(const ScaleConfig& scale,
                     SerialTransport* transport,
                     const ImuFrame& frame,
                     bool loop);

    // Thread-safe update of the live frame in manual mode
    void setLiveFrame(const ImuFrame& f);

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
    SerialTransport*  m_transport   = nullptr;
    bool              m_loop        = true;
    bool              m_manualMode  = false;
    ImuFrame          m_liveFrame;
    QMutex            m_liveMutex;
    QAtomicInt        m_stop{0};
};
