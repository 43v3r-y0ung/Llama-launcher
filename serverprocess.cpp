#include "serverprocess.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr int kPollIntervalMs = 200;
constexpr qint64 kMaxLogBytes = 64LL * 1024 * 1024; // the log is emptied when it grows past this
constexpr qint64 kAdoptTailBytes = 2LL * 1024 * 1024; // output shown when adopting a running server

// Start time of a process in clock ticks since boot, 0 if it does not exist or is a zombie.
quint64 processStartTicks(qint64 pid)
{
    if (pid <= 0)
        return 0;
    QFile f(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!f.open(QIODevice::ReadOnly))
        return 0;
    const QByteArray stat = f.readAll();
    // The command name in parentheses may contain spaces; the fields after it do not.
    const int close = stat.lastIndexOf(')');
    if (close < 0)
        return 0;
    const QList<QByteArray> fields = stat.mid(close + 2).split(' ');
    // fields[0] is field 3 (state), so field 22 (starttime) is fields[19].
    if (fields.size() < 20 || fields.at(0) == "Z" || fields.at(0) == "X")
        return 0;
    return fields.at(19).toULongLong();
}

QString environmentValue(qint64 pid, const QByteArray &name)
{
    QFile f(QStringLiteral("/proc/%1/environ").arg(pid));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QByteArray prefix = name + '=';
    const QList<QByteArray> entries = f.readAll().split('\0');
    for (const QByteArray &e : entries) {
        if (e.startsWith(prefix))
            return QString::fromUtf8(e.mid(prefix.size()));
    }
    return {};
}

// --- supervisor (runs before QApplication exists; plain POSIX only) ---

volatile pid_t g_child = 0;

void forwardSignal(int sig)
{
    if (g_child > 0)
        ::kill(g_child, sig);
}

void writeStatusFile(const char *path, const char *text)
{
    char tmp[4096];
    std::snprintf(tmp, sizeof tmp, "%s.tmp", path);
    const int fd = ::open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0)
        return;
    [[maybe_unused]] const ssize_t written = ::write(fd, text, std::strlen(text));
    ::close(fd);
    ::rename(tmp, path);
}

} // namespace

ServerProcess::ServerProcess(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ServerProcess::poll);
}

QString ServerProcess::stateDir()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
#else
    const QString base = QDir::homePath() + QStringLiteral("/.local/state");
#endif
    return base + QStringLiteral("/llama-launcher");
}

QString ServerProcess::logFilePath() { return stateDir() + QStringLiteral("/server.log"); }
QString ServerProcess::statusFilePath() { return stateDir() + QStringLiteral("/server.status"); }
QString ServerProcess::stateFilePath() { return stateDir() + QStringLiteral("/server.json"); }

bool ServerProcess::start(const Info &info, const QProcessEnvironment &env, QString *error)
{
    if (isRunning()) {
        *error = tr("the server is already running");
        return false;
    }
    if (!QDir().mkpath(stateDir())) {
        *error = tr("cannot create %1").arg(stateDir());
        return false;
    }
    QFile log(logFilePath());
    if (!log.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = tr("cannot write %1: %2").arg(log.fileName(), log.errorString());
        return false;
    }
    log.close();
    QFile::remove(statusFilePath());

    // The supervisor is the launcher binary itself. If it has been replaced by a newer build since
    // it was started, /proc/self/exe (resolved in the new process) still runs the current one.
    QString self = QCoreApplication::applicationFilePath();
    if (!QFileInfo(self).isExecutable())
        self = QStringLiteral("/proc/self/exe");
    QProcess p;
    p.setProgram(self);
    p.setArguments(QStringList{QStringLiteral("--supervise"), statusFilePath(), info.exe} + info.args);
    p.setProcessEnvironment(env);
    p.setStandardInputFile(QProcess::nullDevice());
    // Append mode (O_APPEND) lets the launcher empty a log that grew too large while the server writes.
    p.setStandardOutputFile(logFilePath(), QIODevice::Append);
    p.setStandardErrorFile(logFilePath(), QIODevice::Append);
    qint64 pid = 0;
    if (!p.startDetached(&pid) || pid <= 0) {
        *error = p.errorString();
        return false;
    }

    m_info = info;
    m_info.startedAt = QDateTime::currentDateTime();
    m_supervisorPid = pid;
    m_supervisorStart = processStartTicks(pid); // 0 if it already ended: the first poll reports that
    m_serverPid = 0;
    m_logOffset = 0;
    m_partialLine.clear();
    writeState();
    m_pollTimer.start();
    return true;
}

bool ServerProcess::adopt(QStringList *recentOutput)
{
    if (isRunning())
        return false;
    QFile f(stateFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    const qint64 pid = o.value(QLatin1String("supervisorPid")).toInteger();
    const quint64 started = o.value(QLatin1String("supervisorStart")).toString().toULongLong();
    if (pid <= 0 || started == 0 || processStartTicks(pid) != started) {
        QFile::remove(stateFilePath()); // left behind by a server that has ended
        return false;
    }

    m_supervisorPid = pid;
    m_supervisorStart = started;
    m_info = Info();
    m_info.exe = o.value(QLatin1String("exe")).toString();
    for (const QJsonValue &v : o.value(QLatin1String("args")).toArray())
        m_info.args << v.toString();
    m_info.modelName = o.value(QLatin1String("modelName")).toString();
    m_info.port = o.value(QLatin1String("port")).toInt(8080);
    m_info.webUi = o.value(QLatin1String("webUi")).toBool(true);
    m_info.lan = o.value(QLatin1String("lan")).toBool();
    m_info.https = o.value(QLatin1String("https")).toBool();
    m_info.certPem = o.value(QLatin1String("certPem")).toString().toLatin1();
    m_info.startedAt = QDateTime::fromString(o.value(QLatin1String("startedAt")).toString(), Qt::ISODate);

    int exitCode = -1, crashSignal = 0;
    readStatus(&exitCode, &crashSignal);
    if (m_serverPid > 0)
        m_info.apiKey = environmentValue(m_serverPid, "LLAMA_API_KEY");

    // Show the tail of what the server has written so far, then follow it from the end.
    QFile log(logFilePath());
    if (log.open(QIODevice::ReadOnly)) {
        const qint64 size = log.size();
        const qint64 from = qMax<qint64>(0, size - kAdoptTailBytes);
        log.seek(from);
        QByteArray data = log.read(size - from);
        if (from > 0) // starts mid-line
            data.remove(0, data.indexOf('\n') + 1);
        const int lastNewline = data.lastIndexOf('\n');
        m_partialLine = data.mid(lastNewline + 1);
        const QList<QByteArray> lines = data.left(qMax(0, lastNewline)).split('\n');
        for (const QByteArray &line : lines) {
            const QString trimmed = QString::fromUtf8(line).trimmed();
            if (!trimmed.isEmpty())
                recentOutput->append(trimmed);
        }
        m_logOffset = size;
    }
    m_pollTimer.start();
    return true;
}

void ServerProcess::terminate()
{
    if (!supervisorAlive())
        return;
    // The supervisor passes SIGTERM on, so it also works before the server PID is known.
    ::kill(pid_t(m_serverPid > 0 ? m_serverPid : m_supervisorPid), SIGTERM);
}

void ServerProcess::kill()
{
    if (!supervisorAlive())
        return;
    if (m_serverPid > 0)
        ::kill(pid_t(m_serverPid), SIGKILL);
    else
        ::kill(pid_t(m_supervisorPid), SIGKILL); // the server gets SIGTERM when its parent dies
}

void ServerProcess::stopAndWait(int terminateTimeoutMs)
{
    if (!isRunning())
        return;
    if (m_serverPid == 0) {
        int code, sig;
        readStatus(&code, &sig);
    }
    terminate();
    for (int waited = 0; supervisorAlive() && waited < terminateTimeoutMs; waited += 50)
        QThread::msleep(50);
    if (supervisorAlive()) {
        kill();
        for (int waited = 0; supervisorAlive() && waited < 2000; waited += 50)
            QThread::msleep(50);
    }
    m_pollTimer.stop();
    QFile::remove(stateFilePath());
    reset();
}

bool ServerProcess::supervisorAlive() const
{
    return m_supervisorPid > 0 && m_supervisorStart != 0 && processStartTicks(m_supervisorPid) == m_supervisorStart;
}

void ServerProcess::poll()
{
    if (m_serverPid == 0) {
        int code, sig;
        readStatus(&code, &sig);
        if (m_serverPid > 0)
            emit serverPidKnown(m_serverPid);
    }
    readLog();
    if (supervisorAlive())
        return;

    readLog();
    if (!m_partialLine.trimmed().isEmpty())
        emit outputLines({QString::fromUtf8(m_partialLine).trimmed()});
    int exitCode = -1, crashSignal = 0;
    readStatus(&exitCode, &crashSignal);
    m_pollTimer.stop();
    QFile::remove(stateFilePath());
    QFile::remove(statusFilePath());
    reset();
    emit finished(exitCode, crashSignal);
}

void ServerProcess::readLog()
{
    QFile log(logFilePath());
    if (!log.open(QIODevice::ReadOnly))
        return;
    if (log.size() < m_logOffset) // emptied by someone else
        m_logOffset = 0;
    log.seek(m_logOffset);
    const QByteArray data = log.readAll();
    m_logOffset += data.size();
    log.close();
    if (m_logOffset > kMaxLogBytes) {
        // The server keeps appending at the (new) end of the file.
        if (QFile::resize(logFilePath(), 0))
            m_logOffset = 0;
    }
    if (data.isEmpty())
        return;

    // Only complete lines; a chunk may end in the middle of one.
    m_partialLine += data;
    const int lastNewline = m_partialLine.lastIndexOf('\n');
    if (lastNewline < 0)
        return;
    const QList<QByteArray> raw = m_partialLine.left(lastNewline).split('\n');
    m_partialLine.remove(0, lastNewline + 1);
    QStringList lines;
    for (const QByteArray &line : raw) {
        const QString trimmed = QString::fromUtf8(line).trimmed();
        if (!trimmed.isEmpty())
            lines << trimmed;
    }
    if (!lines.isEmpty())
        emit outputLines(lines);
}

void ServerProcess::readStatus(int *exitCode, int *crashSignal)
{
    QFile f(statusFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QList<QByteArray> lines = f.readAll().split('\n');
    for (const QByteArray &line : lines) {
        const QList<QByteArray> kv = line.split(' ');
        if (kv.size() != 2)
            continue;
        if (kv.at(0) == "pid" && m_serverPid == 0)
            m_serverPid = kv.at(1).toLongLong();
        else if (kv.at(0) == "exit")
            *exitCode = kv.at(1).toInt();
        else if (kv.at(0) == "signal")
            *crashSignal = kv.at(1).toInt();
    }
}

void ServerProcess::writeState() const
{
    QJsonArray args;
    for (const QString &a : m_info.args)
        args.append(a);
    const QJsonObject o{
        {QStringLiteral("supervisorPid"), m_supervisorPid},
        {QStringLiteral("supervisorStart"), QString::number(m_supervisorStart)},
        {QStringLiteral("exe"), m_info.exe},
        {QStringLiteral("args"), args},
        {QStringLiteral("modelName"), m_info.modelName},
        {QStringLiteral("port"), m_info.port},
        {QStringLiteral("webUi"), m_info.webUi},
        {QStringLiteral("lan"), m_info.lan},
        {QStringLiteral("https"), m_info.https},
        {QStringLiteral("certPem"), QString::fromLatin1(m_info.certPem)},
        {QStringLiteral("startedAt"), m_info.startedAt.toString(Qt::ISODate)},
    };
    QSaveFile f(stateFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(o).toJson());
        f.commit();
    }
}

void ServerProcess::reset()
{
    m_supervisorPid = 0;
    m_supervisorStart = 0;
    m_serverPid = 0;
    m_logOffset = 0;
    m_partialLine.clear();
    m_info = Info();
}

// ---------------------------------------------------------------------------
// Supervisor
// ---------------------------------------------------------------------------

bool ServerProcess::isSupervisorInvocation(int argc, char **argv)
{
    return argc >= 4 && std::strcmp(argv[1], "--supervise") == 0;
}

int ServerProcess::runSupervisor(int argc, char **argv)
{
    (void)argc;
    const char *statusPath = argv[2];
    char **command = argv + 3;

    ::setsid(); // no controlling terminal: closing the launcher's terminal does not end the server
    ::close_range(3, ~0U, 0); // nothing of the launcher's (sockets, files) leaks into the server

    const pid_t self = ::getpid();
    const pid_t child = ::fork();
    if (child < 0) {
        std::perror("fork");
        return 1;
    }
    if (child == 0) {
        // If the supervisor is killed, the server stops too instead of running unobserved.
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (::getppid() != self)
            ::_exit(1);
        sigset_t none;
        sigemptyset(&none);
        ::sigprocmask(SIG_SETMASK, &none, nullptr);
        ::signal(SIGHUP, SIG_DFL);
        ::execvp(command[0], command);
        std::fprintf(stderr, "Cannot run %s: %s\n", command[0], std::strerror(errno));
        ::_exit(127);
    }

    g_child = child;
    struct sigaction action = {};
    action.sa_handler = forwardSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGTERM, &action, nullptr);
    ::sigaction(SIGINT, &action, nullptr);
    ::signal(SIGHUP, SIG_IGN);

    char text[128];
    std::snprintf(text, sizeof text, "pid %d\n", int(child));
    writeStatusFile(statusPath, text);

    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFSIGNALED(status))
        std::snprintf(text, sizeof text, "pid %d\nsignal %d\n", int(child), WTERMSIG(status));
    else
        std::snprintf(text, sizeof text, "pid %d\nexit %d\n", int(child), WEXITSTATUS(status));
    writeStatusFile(statusPath, text);
    return 0;
}
