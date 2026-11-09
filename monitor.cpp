#include "monitor.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>

#include <unistd.h>

namespace {

constexpr int kIntervalMs = 1000;
constexpr int kGpuBusyIntervalMs = 200;
constexpr qint64 kPromptStaleMs = 5000; // progress lines come every few seconds while processing
constexpr double kGenSmoothing = 0.5;   // weight of the newest reading in the generation speed

// Reads a small /proc or /sys file. Empty on failure (amdgpu answers EBUSY while the GPU sleeps).
QByteArray readSmallFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll().trimmed();
}

qint64 readNumber(const QString &path, qint64 fallback = -1)
{
    bool ok = false;
    const qint64 v = readSmallFile(path).toLongLong(&ok);
    return ok ? v : fallback;
}

// "MemTotal:       32562416 kB" -> bytes
qint64 meminfoValue(const QByteArray &meminfo, const QByteArray &key)
{
    const int at = meminfo.indexOf(key + ':');
    if (at < 0)
        return -1;
    const int end = meminfo.indexOf('\n', at);
    const QList<QByteArray> parts = meminfo.mid(at + key.size() + 1, end < 0 ? -1 : end - at - key.size() - 1)
                                        .simplified().split(' ');
    bool ok = false;
    const qint64 kb = parts.value(0).toLongLong(&ok);
    return ok ? kb * 1024 : -1;
}

} // namespace

Monitor::Monitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_network(new QNetworkAccessManager(this))
    , m_gpuBusyTimer(new QTimer(this))
{
    m_timer->setInterval(kIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &Monitor::sample);
    m_gpuBusyTimer->setInterval(kGpuBusyIntervalMs);
    connect(m_gpuBusyTimer, &QTimer::timeout, this, &Monitor::readGpuBusy);
    findAmdGpu();
}

void Monitor::start()
{
    if (m_timer->isActive())
        return;
    // A gap in sampling makes the next CPU delta span the gap; start over instead.
    m_prevCpuTotal = m_prevCpuBusy = 0;
    m_prevProcTicks = 0;
    m_prevProcMs = 0;
    m_gpuBusySum = 0;
    m_gpuBusyCount = 0;
    m_timer->start();
    if (!m_gpuDevice.isEmpty())
        m_gpuBusyTimer->start();
    sample();
}

void Monitor::stop()
{
    m_timer->stop();
    m_gpuBusyTimer->stop();
}

bool Monitor::isActive() const
{
    return m_timer->isActive();
}

void Monitor::setProcessId(qint64 pid)
{
    m_pid = pid;
    m_prevProcTicks = 0;
    m_prevProcMs = 0;
}

void Monitor::setServer(const QUrl &baseUrl, const QString &apiKey)
{
    m_serverUrl = baseUrl;
    m_apiKey = apiKey;
    m_prevDecoded = -1;
    m_genTps = 0;
    m_busy = false;
    if (baseUrl.isEmpty()) {
        if (m_slotsReply)
            m_slotsReply->abort();
        m_ctxUsed = m_ctxSize = -1;
    }
}

void Monitor::resetServerStats()
{
    m_genTps = 0;
    m_busy = false;
    m_ctxUsed = m_ctxSize = -1;
    m_prevDecoded = -1;
    m_promptTps = 0;
    m_promptProgress = -1;
    m_lastGenTps = m_lastPromptTps = -1;
}

// ---------------------------------------------------------------------------
// Sampling
// ---------------------------------------------------------------------------

void Monitor::sample()
{
    MonitorSample s;
    s.timeMs = QDateTime::currentMSecsSinceEpoch();
    sampleCpu(s);
    sampleMemory(s);
    sampleProcess(s, s.timeMs);
    sampleGpu(s);

    const bool serverUp = m_pid > 0;
    if (serverUp) {
        // Prompt speed comes from progress lines in the log; drop it once they stop coming.
        const bool prompting = m_promptProgress >= 0 && s.timeMs - m_promptSeenMs < kPromptStaleMs;
        s.promptTps = prompting ? m_promptTps : 0;
        s.promptProgress = prompting ? m_promptProgress : -1;
        s.genTps = m_serverUrl.isEmpty() ? 0 : m_genTps;
        s.busy = m_busy || prompting;
        s.ctxUsed = m_ctxUsed;
        s.ctxSize = m_ctxSize;
        s.lastGenTps = m_lastGenTps;
        s.lastPromptTps = m_lastPromptTps;
    }

    m_history.append(s);
    if (m_history.size() > kHistorySize)
        m_history.removeFirst();
    emit sampled(s);

    if (!m_serverUrl.isEmpty())
        pollSlots();
}

void Monitor::sampleCpu(MonitorSample &s)
{
    // First line of /proc/stat: "cpu  user nice system idle iowait irq softirq steal ..."
    const QByteArray stat = readSmallFile(QStringLiteral("/proc/stat"));
    const int eol = stat.indexOf('\n');
    const QList<QByteArray> f = stat.left(eol).simplified().split(' ');
    if (f.size() < 8 || f.at(0) != "cpu")
        return;
    quint64 total = 0;
    for (int i = 1; i < qMin<int>(f.size(), 9); ++i) // up to steal; guest time is already in user
        total += f.at(i).toULongLong();
    const quint64 idle = f.at(4).toULongLong() + f.at(5).toULongLong();
    const quint64 busy = total - idle;

    if (m_prevCpuTotal && total > m_prevCpuTotal)
        s.cpuPercent = 100.0 * double(busy - m_prevCpuBusy) / double(total - m_prevCpuTotal);
    m_prevCpuTotal = total;
    m_prevCpuBusy = busy;
}

void Monitor::sampleMemory(MonitorSample &s)
{
    const QByteArray meminfo = readSmallFile(QStringLiteral("/proc/meminfo"));
    s.ramTotal = meminfoValue(meminfo, "MemTotal");
    const qint64 available = meminfoValue(meminfo, "MemAvailable");
    if (s.ramTotal > 0 && available >= 0)
        s.ramUsed = s.ramTotal - available;
}

void Monitor::sampleProcess(MonitorSample &s, qint64 nowMs)
{
    if (m_pid <= 0)
        return;
    const QString base = QStringLiteral("/proc/%1/").arg(m_pid);

    const QByteArray status = readSmallFile(base + QStringLiteral("status"));
    s.procRss = meminfoValue(status, "VmRSS");

    // /proc/<pid>/stat: the command name is in parentheses and may contain spaces;
    // utime and stime are fields 14 and 15, i.e. 12th and 13th after the closing parenthesis.
    const QByteArray stat = readSmallFile(base + QStringLiteral("stat"));
    const int paren = stat.lastIndexOf(')');
    if (paren < 0)
        return;
    const QList<QByteArray> f = stat.mid(paren + 2).split(' ');
    if (f.size() < 13)
        return;
    const quint64 ticks = f.at(11).toULongLong() + f.at(12).toULongLong();

    static const long ticksPerSec = sysconf(_SC_CLK_TCK);
    static const int cpus = qMax(1, QThread::idealThreadCount());
    if (m_prevProcMs && nowMs > m_prevProcMs && ticks >= m_prevProcTicks) {
        const double cpuSeconds = double(ticks - m_prevProcTicks) / double(ticksPerSec);
        const double wallSeconds = double(nowMs - m_prevProcMs) / 1000.0;
        s.procCpuPercent = qBound(0.0, 100.0 * cpuSeconds / (wallSeconds * cpus), 100.0);
    }
    m_prevProcTicks = ticks;
    m_prevProcMs = nowMs;
}

void Monitor::findAmdGpu()
{
    // Pick the AMD card with the most VRAM (an AMD APU would report only its small carve-out).
    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QRegularExpression cardName(QStringLiteral("^card\\d+$"));
    qint64 bestVram = 0;
    for (const QString &card : drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!cardName.match(card).hasMatch())
            continue;
        const QString dev = drm.absoluteFilePath(card) + QStringLiteral("/device");
        if (readSmallFile(dev + QStringLiteral("/vendor")) != "0x1002")
            continue;
        const qint64 vram = readNumber(dev + QStringLiteral("/mem_info_vram_total"), 0);
        if (vram <= bestVram)
            continue;
        bestVram = vram;
        m_gpuDevice = dev;
        const QStringList hwmons = QDir(dev + QStringLiteral("/hwmon")).entryList({QStringLiteral("hwmon*")}, QDir::Dirs);
        m_gpuHwmon = hwmons.isEmpty() ? QString() : dev + QStringLiteral("/hwmon/") + hwmons.first();
    }
    if (!m_gpuDevice.isEmpty()) {
        const QString product = QString::fromUtf8(readSmallFile(m_gpuDevice + QStringLiteral("/product_name")));
        m_gpuName = product.isEmpty() ? QStringLiteral("AMD GPU") : product;
    }
}

void Monitor::sampleGpu(MonitorSample &s)
{
    if (m_gpuDevice.isEmpty())
        return;
    s.gpuPresent = true;
    const QString &dev = m_gpuDevice;

    s.vramTotal = readNumber(dev + QStringLiteral("/mem_info_vram_total"));
    s.vramUsed = readNumber(dev + QStringLiteral("/mem_info_vram_used"));
    s.gttTotal = readNumber(dev + QStringLiteral("/mem_info_gtt_total"));
    s.gttUsed = readNumber(dev + QStringLiteral("/mem_info_gtt_used"));

    readGpuBusy();
    if (m_gpuBusyCount > 0)
        s.gpuBusy = m_gpuBusySum / m_gpuBusyCount;
    m_gpuBusySum = 0;
    m_gpuBusyCount = 0;

    // A runtime-suspended card is idle; its sensors refuse to be read (and must not be woken up).
    s.gpuSuspended = readSmallFile(dev + QStringLiteral("/power/runtime_status")) == "suspended";
    if (s.gpuSuspended) {
        s.gpuBusy = 0;
        return;
    }
    if (m_gpuHwmon.isEmpty())
        return;
    if (const qint64 t = readNumber(m_gpuHwmon + QStringLiteral("/temp1_input")); t >= 0)
        s.gpuTempC = t / 1000.0;
    qint64 power = readNumber(m_gpuHwmon + QStringLiteral("/power1_average"));
    if (power < 0)
        power = readNumber(m_gpuHwmon + QStringLiteral("/power1_input"));
    if (power >= 0)
        s.gpuPowerW = power / 1e6;
    if (const qint64 hz = readNumber(m_gpuHwmon + QStringLiteral("/freq1_input")); hz >= 0)
        s.gpuClockMHz = int(hz / 1000000);
}

void Monitor::readGpuBusy()
{
    // Readings fail (EBUSY) while the card is runtime-suspended; those are simply skipped.
    const qint64 busy = readNumber(m_gpuDevice + QStringLiteral("/gpu_busy_percent"));
    if (busy >= 0) {
        m_gpuBusySum += double(busy);
        ++m_gpuBusyCount;
    }
}

// ---------------------------------------------------------------------------
// llama-server
// ---------------------------------------------------------------------------

void Monitor::pollSlots()
{
    if (m_slotsReply)
        return; // previous poll still pending
    QNetworkRequest request(m_serverUrl.resolved(QUrl(QStringLiteral("slots"))));
    request.setTransferTimeout(kIntervalMs - 100);
    if (!m_apiKey.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    m_slotsReply = m_network->get(request);
    QNetworkReply *reply = m_slotsReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleSlotsReply(reply); });
}

void Monitor::handleSlotsReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply == m_slotsReply)
        m_slotsReply = nullptr;
    if (reply->error() != QNetworkReply::NoError || m_serverUrl.isEmpty())
        return;

    const QJsonArray slotList = QJsonDocument::fromJson(reply->readAll()).array();
    if (slotList.isEmpty())
        return;

    qint64 decoded = 0;
    bool busy = false;
    double fullest = -1;
    for (const QJsonValue &v : slotList) {
        const QJsonObject slot = v.toObject();
        const bool processing = slot.value(QLatin1String("is_processing")).toBool();
        busy |= processing;
        // Only running slots count: an idle slot keeps the n_decoded of its last request.
        if (processing)
            decoded += slot.value(QLatin1String("next_token")).toArray().at(0).toObject()
                           .value(QLatin1String("n_decoded")).toInteger();

        // Generated tokens are appended to the slot's prompt, so n_prompt_tokens is the context in use.
        const int ctx = slot.value(QLatin1String("n_ctx")).toInt();
        const int used = slot.value(QLatin1String("n_prompt_tokens")).toInt(-1);
        if (ctx > 0 && used >= 0 && double(used) / ctx > fullest) {
            fullest = double(used) / ctx;
            m_ctxUsed = used;
            m_ctxSize = ctx;
        } else if (ctx > 0 && m_ctxSize < 0) {
            m_ctxUsed = 0;
            m_ctxSize = ctx;
        }
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!busy) {
        m_genTps = 0;
    } else if (m_prevDecoded >= 0 && now > m_prevDecodedMs) {
        // n_decoded restarts at 0 with each request; a drop means a new one began since the last poll.
        const qint64 delta = decoded >= m_prevDecoded ? decoded - m_prevDecoded : decoded;
        const double tps = double(delta) * 1000.0 / double(now - m_prevDecodedMs);
        // Whole tokens per ~1 s poll alternate (e.g. 9, 10, 9); smooth that out.
        m_genTps = m_genTps > 0 ? kGenSmoothing * tps + (1 - kGenSmoothing) * m_genTps : tps;
    }
    m_prevDecoded = busy ? decoded : -1;
    // The final "prompt eval time" line is only printed when the whole request ends;
    // the first generated token already means the prompt is done.
    if (decoded > 0)
        m_promptProgress = -1;
    m_prevDecodedMs = now;
    m_busy = busy;
}

void Monitor::feedLogLine(const QString &line)
{
    // slot update_slots: id  0 | task 5 | prompt processing, n_tokens =    512, progress = 0.25, t =   3.10 s / 165.16 tokens per second
    static const QRegularExpression progressRe(
        QStringLiteral("prompt processing, n_tokens =\\s*\\d+, progress =\\s*([\\d.]+), t =\\s*[\\d.]+ s /\\s*([\\d.]+) tokens per second"));
    //  prompt eval time =     123.45 ms /    10 tokens (   12.35 ms per token,    81.00 tokens per second)
    //         eval time =    1234.56 ms /    50 tokens (   24.69 ms per token,    40.50 tokens per second)
    static const QRegularExpression timingRe(
        QStringLiteral("(prompt )?eval time =\\s*[\\d.]+ ms /\\s*\\d+ tokens \\(\\s*[\\d.]+ ms per token,\\s*([\\d.]+) tokens per second\\)"));

    if (!line.contains(QLatin1String("tokens per second")))
        return;

    if (const QRegularExpressionMatch m = progressRe.match(line); m.hasMatch()) {
        m_promptProgress = m.captured(1).toDouble();
        m_promptTps = m.captured(2).toDouble();
        m_promptSeenMs = QDateTime::currentMSecsSinceEpoch();
        return;
    }
    if (const QRegularExpressionMatch m = timingRe.match(line); m.hasMatch()) {
        const double tps = m.captured(2).toDouble();
        if (m.capturedLength(1) > 0) {
            m_lastPromptTps = tps;
            m_promptProgress = -1; // prompt done, generation follows
        } else {
            m_lastGenTps = tps;
        }
    }
}
