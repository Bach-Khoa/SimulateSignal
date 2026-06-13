#include "simulatorengine.h"
#include <QThread>
#include <QMutexLocker>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <time.h>
#endif

static constexpr int FREQ_HZ     = 125;
static constexpr int STATS_EVERY = 12;   // emit statsUpdated every N frames (~96ms)
static constexpr int LOG_EVERY   = 64;   // emit packetReady every N frames (~5Hz)

SimulatorEngine::SimulatorEngine(QObject* parent) : QThread(parent) {}

void SimulatorEngine::setup(const QVector<ImuFrame>& frames,
                            const ScaleConfig& scale,
                            SerialTransport* transport,
                            bool loop)
{
    m_frames     = frames;
    m_scale      = scale;
    m_transport  = transport;
    m_loop       = loop;
    m_manualMode = false;
    m_stop.storeRelease(0);
}

void SimulatorEngine::setupManual(const ScaleConfig& scale,
                                  SerialTransport* transport,
                                  const ImuFrame& frame,
                                  bool loop)
{
    m_scale      = scale;
    m_transport  = transport;
    m_loop       = loop;
    m_manualMode = true;
    m_liveFrame  = frame;
    m_stop.storeRelease(0);
}

void SimulatorEngine::setLiveFrame(const ImuFrame& f) {
    QMutexLocker lk(&m_liveMutex);
    m_liveFrame = f;
}

void SimulatorEngine::requestStop() { m_stop.storeRelease(1); }

void SimulatorEngine::run() {
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

    if (!m_manualMode && m_frames.isEmpty()) {
        emit errorOccurred("No frames loaded.");
        emit finished();
        return;
    }

#ifdef Q_OS_WIN
    LARGE_INTEGER freq, now, next;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&next);
    const LONGLONG ticksPerFrame = freq.QuadPart / FREQ_HZ;
    next.QuadPart += ticksPerFrame;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t nextNs = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec
                     + (1000000000LL / FREQ_HZ);
#endif

    int      idx       = 0;
    qint64   sentTotal = 0;
    int      total     = m_manualMode ? 0 : m_frames.size();

    while (!m_stop.loadAcquire()) {
        ImuFrame frame;
        if (m_manualMode) {
            QMutexLocker lk(&m_liveMutex);
            frame = m_liveFrame;
        } else {
            frame = m_frames[idx % total];
        }

        QByteArray pkt = PacketBuilder5A::build(frame, m_scale);

        if (!m_transport->send(pkt)) {
            emit errorOccurred(m_transport->lastError());
            break;
        }

        sentTotal++;
        if (!m_manualMode) idx++;

        const bool isLastPacket = ( m_manualMode && !m_loop) ||
                                  (!m_manualMode && !m_loop && idx >= total);

        if (sentTotal % STATS_EVERY == 0 || isLastPacket)
            emit statsUpdated(m_manualMode ? 0 : (total > 0 ? idx % total : 0), total, sentTotal);

        if (sentTotal % LOG_EVERY == 0 || isLastPacket)
            emit packetReady(pkt, frame);

        if (isLastPacket) break;

        // High-precision busy-wait until next frame boundary
#ifdef Q_OS_WIN
        do { QueryPerformanceCounter(&now); } while (now.QuadPart < next.QuadPart);
        next.QuadPart += ticksPerFrame;
#else
        do { clock_gettime(CLOCK_MONOTONIC, &ts); }
        while ((int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec < nextNs);
        nextNs += 1000000000LL / FREQ_HZ;
#endif
    }

    emit statsUpdated(m_manualMode ? 0 : (idx % (total > 0 ? total : 1)), total, sentTotal);
    emit finished();
}
