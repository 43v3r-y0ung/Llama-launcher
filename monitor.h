#ifndef MONITOR_H
#define MONITOR_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// One reading of everything the launcher monitors. A value of -1 means "not available".
struct MonitorSample
{
    qint64 timeMs = 0;              // QDateTime::currentMSecsSinceEpoch()

    // System
    double cpuPercent = -1;         // whole machine, 0..100
    qint64 ramUsed = -1;            // bytes, MemTotal - MemAvailable
    qint64 ramTotal = -1;

    // llama-server process
    double procCpuPercent = -1;     // share of the whole machine, 0..100
    qint64 procRss = -1;            // bytes

    // AMD GPU (amdgpu sysfs)
    bool gpuPresent = false;
    bool gpuSuspended = false;      // runtime-suspended: idle, sensors unreadable
    double gpuBusy = -1;            // 0..100
    double gpuTempC = -1;
    double gpuPowerW = -1;
    int gpuClockMHz = -1;
    qint64 vramUsed = -1, vramTotal = -1;
    qint64 gttUsed = -1, gttTotal = -1;   // system RAM mapped for the GPU

    // llama-server inference
    double genTps = -1;             // tokens/s generated during the last second (0 when idle)
    double promptTps = -1;          // prompt processing speed while a prompt is processed (0 otherwise)
    double promptProgress = -1;     // 0..1 while a prompt is processed
    double lastGenTps = -1;         // final speeds of the last finished request, from the log
    double lastPromptTps = -1;
    bool busy = false;              // a slot is processing a request
    int ctxUsed = -1, ctxSize = -1; // tokens in the fullest slot
};

// Samples system, AMD GPU and llama-server statistics once per second and keeps a short history.
// Everything is read from /proc and /sys without root; the server is queried via GET /slots.
class Monitor : public QObject
{
    Q_OBJECT

public:
    static constexpr int kHistorySize = 300; // samples (= seconds)

    explicit Monitor(QObject *parent = nullptr);

    void start();
    void stop();
    bool isActive() const;

    // llama-server process to account CPU/RAM for (0 = none).
    void setProcessId(qint64 pid);
    // Server to poll for slot statistics once it answers /health; empty URL stops polling.
    void setServer(const QUrl &baseUrl, const QString &apiKey);
    // Server output, one line at a time; picks up throughput reports.
    void feedLogLine(const QString &line);
    // Forget the inference figures (new server run).
    void resetServerStats();

    const QList<MonitorSample> &history() const { return m_history; }
    MonitorSample latest() const { return m_history.isEmpty() ? MonitorSample() : m_history.constLast(); }
    QString gpuName() const { return m_gpuName; }

signals:
    void sampled(const MonitorSample &sample);

private:
    void sample();
    void sampleCpu(MonitorSample &s);
    void sampleMemory(MonitorSample &s);
    void sampleProcess(MonitorSample &s, qint64 nowMs);
    void sampleGpu(MonitorSample &s);
    void pollSlots();
    void handleSlotsReply(QNetworkReply *reply);
    void findAmdGpu();
    void readGpuBusy();

    QTimer *m_timer;
    QNetworkAccessManager *m_network;
    QList<MonitorSample> m_history;

    // CPU deltas
    quint64 m_prevCpuTotal = 0, m_prevCpuBusy = 0;
    qint64 m_pid = 0;
    quint64 m_prevProcTicks = 0;
    qint64 m_prevProcMs = 0;

    // AMD GPU sysfs paths, empty if none
    QString m_gpuDevice;   // /sys/class/drm/cardN/device
    QString m_gpuHwmon;    // .../hwmon/hwmonM
    QString m_gpuName;
    // gpu_busy_percent is an instantaneous reading (on Polaris it flips between 0 and 100),
    // so it is read several times per sample and averaged.
    QTimer *m_gpuBusyTimer;
    double m_gpuBusySum = 0;
    int m_gpuBusyCount = 0;

    // Server polling
    QUrl m_serverUrl;
    QString m_apiKey;
    QNetworkReply *m_slotsReply = nullptr;
    qint64 m_prevDecoded = -1;      // sum of n_decoded over slots at the previous poll
    qint64 m_prevDecodedMs = 0;
    double m_genTps = 0;
    bool m_busy = false;
    int m_ctxUsed = -1, m_ctxSize = -1;

    // From the log
    double m_promptTps = 0;
    double m_promptProgress = -1;
    qint64 m_promptSeenMs = 0;      // last progress line; stale after a few seconds
    double m_lastGenTps = -1, m_lastPromptTps = -1;
    // "n_gen = ..., tg_3s = ... t/s" progress lines, per slot: the server's own measurement of the
    // last ~3 s. Printed only after 100 tokens and at most every 3 s; /slots covers the start.
    struct GenProgress { double tps; qint64 seenMs; };
    QHash<int, GenProgress> m_genProgress;
};

#endif // MONITOR_H
