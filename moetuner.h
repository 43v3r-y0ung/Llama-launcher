#ifndef MOETUNER_H
#define MOETUNER_H

#include <QDialog>
#include <QElapsedTimer>
#include <QList>
#include <QStringList>

class QLabel;
class QProcess;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTimer;

// Finds the --n-cpu-moe value with the fastest generation by running llama-bench: starts with all
// expert layers on the CPU (always fits), moves layers to the GPU step by step until the GPU runs
// out of memory or generation gets slower, then refines around the best value.
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
    };
    enum class Phase { Idle, Coarse, Refine, Done };

    void start();
    void runNext();
    void runOne(int n);
    void onFinished(int exitCode);
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
};

#endif // MOETUNER_H
