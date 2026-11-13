#ifndef SERVERPROCESS_H
#define SERVERPROCESS_H

#include <QDateTime>
#include <QObject>
#include <QProcessEnvironment>
#include <QStringList>
#include <QTimer>

// llama-server running independently of the launcher, so the launcher can be restarted or
// upgraded while the server keeps serving requests.
//
// The launcher starts a copy of itself in supervisor mode (see runSupervisor()), detached and in
// its own session. The supervisor starts llama-server, waits for it and records its exit status;
// the server's output goes to a log file that the launcher follows. What the launcher needs to know
// about the run is kept in a small state file, so the next launcher can adopt the server.
class ServerProcess : public QObject
{
    Q_OBJECT

public:
    // What the server was started with. The API key is not stored on disk: an adopting launcher
    // reads it from the server's environment.
    struct Info
    {
        QString exe;
        QStringList args;
        QString apiKey;
        QString modelName;
        int port = 8080;
        bool webUi = true;
        bool lan = false;
        bool https = false;
        QString certFile;
        QDateTime startedAt;
    };

    explicit ServerProcess(QObject *parent = nullptr);

    bool isRunning() const { return m_supervisorPid > 0; }
    qint64 serverPid() const { return m_serverPid; } // 0 until the supervisor has reported it
    const Info &info() const { return m_info; }

    bool start(const Info &info, const QProcessEnvironment &env, QString *error);
    // Attaches to a server left running by a previous launcher. Returns its recent output.
    bool adopt(QStringList *recentOutput);
    void terminate();
    void kill();
    // Stops the server and waits for it, for when the launcher is about to exit.
    void stopAndWait(int terminateTimeoutMs);

    static QString logFilePath();

    // Supervisor mode: Llama-launcher --supervise <status file> <program> [args...]
    static bool isSupervisorInvocation(int argc, char **argv);
    static int runSupervisor(int argc, char **argv);

signals:
    void serverPidKnown(qint64 pid);
    void outputLines(const QStringList &lines);
    // exitCode is -1 when unknown; crashSignal is the signal that ended the server, 0 if none.
    void finished(int exitCode, int crashSignal);

private:
    void poll();
    void readLog();
    void readStatus(int *exitCode, int *crashSignal);
    bool supervisorAlive() const;
    void writeState() const;
    void reset();

    static QString stateDir();
    static QString statusFilePath();
    static QString stateFilePath();

    QTimer m_pollTimer;
    Info m_info;
    qint64 m_supervisorPid = 0;
    quint64 m_supervisorStart = 0; // start time in clock ticks, guards against PID reuse
    qint64 m_serverPid = 0;
    qint64 m_logOffset = 0;
    QByteArray m_partialLine;
};

#endif // SERVERPROCESS_H
