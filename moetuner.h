#ifndef MOETUNER_H
#define MOETUNER_H

#include <QDialog>
#include <QElapsedTimer>
#include <QList>
#include <QHash>
#include <QStringList>

#include <functional>

class QLabel;
class QProcess;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTimer;

// Finds the --n-cpu-moe value with the fastest generation by running llama-bench: starts with all
// expert layers on the CPU, moves layers to the GPU step by step while the server (with its real
// context size, estimated by llama-fit-params) still fits in GPU memory and generation does not
// get slower, then refines around the best value.
class MoeTuner : public QDialog
{
    Q_OBJECT

public:
    struct Params
    {
        QString benchExe;
        QString model;
        int layers = 0;          // transformer layers (block_count)
        QStringList extraArgs;   // device, flash attention, KV cache... as the server would use them
        // GPU memory check with the server's own context size (llama-bench hardly allocates any):
        QString fitExe;          // llama-fit-params; empty to skip the check
        QStringList fitArgs;     // model, context, slots, device... in llama-server syntax, without -ncmoe
        QString serverExe;       // for --list-devices (free memory per device)
        qint64 extraGpuMiB = 0;  // loaded by the server besides the model (multimodal projector)
    };

    explicit MoeTuner(const Params &params, QWidget *parent = nullptr);
    ~MoeTuner() override;

    int chosenValue() const { return m_chosen; } // -1 unless "Use" was clicked

protected:
    void reject() override;

private:
    struct Result
    {
        int n;
        double promptTps = -1;
        double genTps = -1;
        bool failed = false;
        bool noMemory = false;   // not run: the server would not fit in GPU memory
    };
    struct Estimate
    {
        qint64 needMiB = -1;     // GPU memory the server would use, all devices
        qint64 freeMiB = -1;     // free on those devices, -1 if unknown
        bool fits = true;
    };
    enum class Phase { Idle, Estimate, Coarse, Refine, Done };

    void start();
    void runTool(const QString &exe, const QStringList &args, const std::function<void(bool, const QByteArray &)> &done);
    void readFreeMemory();
    void estimate(int n, const std::function<void()> &done);
    void findMinimum(int lo, int hi);
    void startBenchmarks();
    void runNext();
    void runOne(int n);
    void onFinished(bool ok, const QByteArray &out);
    void finish(const QString &message);
    void addRow(const Result &r);
    const Result *best() const;
    bool tested(int n) const;

    Params m_params;
    QLabel *m_info;
    QTableWidget *m_table;
    QProgressBar *m_progress;
    QLabel *m_status;
    QPushButton *m_startButton;
    QPushButton *m_useButton;
    QProcess *m_process = nullptr;
    QTimer *m_elapsedTimer;
    QElapsedTimer m_runClock;

    Phase m_phase = Phase::Idle;
    int m_step = 1;
    QList<int> m_queue;          // values still to run in the current phase
    QList<Result> m_results;
    int m_current = -1;          // value being benchmarked
    int m_slowerInARow = 0;
    int m_chosen = -1;
    QString m_stderrTail;        // last lines of llama-bench's log, shown when a run fails
    QHash<QString, qint64> m_freeMiB;  // per device, from --list-devices
    QHash<int, Estimate> m_estimates;  // per number of layers on the CPU
    int m_minFitting = 0;        // fewest layers on the CPU that still fit the server in GPU memory
    QString m_estimateNote;      // why GPU memory was not checked
};

#endif // MOETUNER_H
