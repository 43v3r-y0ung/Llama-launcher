#include "moetuner.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QProcess>
#include <QProgressBar>
#include <QRegularExpression>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace {

// Short runs: the point is comparing settings, not precise figures.
const QStringList kBenchArgs = {QStringLiteral("-p"), QStringLiteral("256"), QStringLiteral("-n"), QStringLiteral("64"),
                                QStringLiteral("-r"), QStringLiteral("2"), QStringLiteral("-o"), QStringLiteral("jsonl")};
constexpr double kSlowerFactor = 0.9; // a result this much below the best counts as slower
constexpr int kTargetCoarseRuns = 12;
// Left free on the GPU: the estimate misses a little (driver, fragmentation), and running out of
// GPU memory does not fail on Vulkan, it silently moves buffers to much slower system memory.
constexpr qint64 kMarginMiB = 256;

QString number(double v, int decimals)
{
    return QLocale().toString(v, 'f', decimals);
}

} // namespace

MoeTuner::MoeTuner(const Params &params, QWidget *parent)
    : QDialog(parent)
    , m_params(params)
    , m_elapsedTimer(new QTimer(this))
{
    setWindowTitle(tr("Find the Fastest MoE Split"));
    resize(560, 480);

    m_info = new QLabel(this);
    m_info->setWordWrap(true);
    m_info->setText(tr("Runs llama-bench with different numbers of expert layers kept on the CPU (--n-cpu-moe), "
                       "starting with all %1 on the CPU and moving them to the GPU as long as the server, with "
                       "its context size, still fits in GPU memory and generation does not get slower. "
                       "Each run loads the model, so this takes a few minutes.")
                        .arg(m_params.layers));

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Layers on CPU"), tr("GPU memory, MiB"), tr("Prompt, tok/s"),
                                        tr("Generation, tok/s")});
    m_table->horizontalHeaderItem(1)->setToolTip(
        tr("What the server would use with its context size, estimated by llama-fit-params, and how much is free."));
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 1);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *buttons = new QDialogButtonBox(this);
    m_startButton = buttons->addButton(tr("Start"), QDialogButtonBox::ActionRole);
    m_useButton = buttons->addButton(tr("Use"), QDialogButtonBox::AcceptRole);
    m_useButton->setEnabled(false);
    buttons->addButton(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &MoeTuner::reject);
    connect(m_startButton, &QPushButton::clicked, this, &MoeTuner::start);
    connect(m_useButton, &QPushButton::clicked, this, [this] {
        const int row = m_table->currentRow();
        if (row < 0)
            return;
        m_chosen = m_table->item(row, 0)->data(Qt::UserRole).toInt();
        accept();
    });
    // The selected row is the one "Use" applies; failed runs cannot be used.
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        const bool usable = row >= 0 && m_phase == Phase::Done && m_table->item(row, 3)->data(Qt::UserRole).toBool();
        m_useButton->setEnabled(usable);
        if (usable)
            m_useButton->setText(tr("Use %1").arg(m_table->item(row, 0)->data(Qt::UserRole).toInt()));
    });

    m_elapsedTimer->setInterval(1000);
    connect(m_elapsedTimer, &QTimer::timeout, this, [this] {
        if (m_current >= 0)
            m_status->setText(tr("Testing %1 layers on the CPU... %2 s").arg(m_current).arg(m_runClock.elapsed() / 1000));
    });

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_info);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_progress);
    layout->addWidget(m_status);
    layout->addWidget(buttons);
}

MoeTuner::~MoeTuner()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void MoeTuner::reject()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(2000);
        m_process->deleteLater();
        m_process = nullptr;
    }
    QDialog::reject();
}

void MoeTuner::start()
{
    m_results.clear();
    m_estimates.clear();
    m_freeMiB.clear();
    m_estimateNote.clear();
    m_minFitting = 0;
    m_table->setRowCount(0);
    m_useButton->setEnabled(false);
    m_useButton->setText(tr("Use"));
    m_startButton->setEnabled(false);
    m_progress->setRange(0, 0); // busy
    m_slowerInARow = 0;

    if (m_params.fitExe.isEmpty()) {
        m_estimateNote = tr("llama-fit-params was not found next to llama-server, so whether the server fits in "
                            "GPU memory with its context size was not checked.");
        startBenchmarks();
        return;
    }
    m_phase = Phase::Estimate;
    m_status->setText(tr("Estimating GPU memory..."));
    readFreeMemory();
}

void MoeTuner::runTool(const QString &exe, const QStringList &args,
                       const std::function<void(bool, const QByteArray &)> &done)
{
    m_stderrTail.clear();
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        // llama.cpp tools log to stderr; keep the end for the error message of a failed run.
        m_stderrTail += QString::fromUtf8(m_process->readAllStandardError());
        if (m_stderrTail.size() > 4000)
            m_stderrTail = m_stderrTail.right(4000);
    });
    connect(m_process, &QProcess::finished, this, [this, done](int code, QProcess::ExitStatus status) {
        const QByteArray out = m_process->readAllStandardOutput();
        m_process->deleteLater();
        m_process = nullptr;
        done(status == QProcess::NormalExit && code == 0, out);
    });
    connect(m_process, &QProcess::errorOccurred, this, [this, exe, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        m_process->deleteLater();
        m_process = nullptr;
        m_stderrTail = tr("Could not run %1.").arg(exe);
        done(false, QByteArray());
    });
    m_process->start(exe, args);
}

void MoeTuner::readFreeMemory()
{
    runTool(m_params.serverExe, {QStringLiteral("--list-devices")}, [this](bool, const QByteArray &out) {
        // "  Vulkan1: AMD Radeon Pro WX Series (RADV POLARIS11) (4096 MiB, 4090 MiB free)"
        static const QRegularExpression re(QStringLiteral("^\\s*(\\S+): .*\\((\\d+) MiB, (\\d+) MiB free\\)\\s*$"),
                                           QRegularExpression::MultilineOption);
        for (auto it = re.globalMatch(QString::fromUtf8(out)); it.hasNext();) {
            const auto m = it.next();
            m_freeMiB.insert(m.captured(1), m.captured(3).toLongLong());
        }
        estimate(m_params.layers, [this] {
            const Estimate e = m_estimates.value(m_params.layers);
            if (e.needMiB < 0) {
                QStringList tail = m_stderrTail.trimmed().split(QLatin1Char('\n'));
                m_estimateNote = tr("llama-fit-params could not estimate GPU memory (%1), so whether the server "
                                    "fits with its context size was not checked.")
                                     .arg(tail.constLast().trimmed());
                startBenchmarks();
            } else if (e.freeMiB < 0) {
                m_estimateNote = tr("Free GPU memory is unknown, so whether the server fits with its context "
                                    "size was not checked.");
                startBenchmarks();
            } else if (!e.fits) {
                finish(tr("Even with all %1 expert layers on the CPU the server needs about %2 MiB of GPU memory, "
                          "and %3 MiB is free. Use a smaller context, a quantized KV cache or keep the KV cache "
                          "in RAM.")
                           .arg(m_params.layers).arg(e.needMiB).arg(e.freeMiB));
            } else {
                findMinimum(0, m_params.layers);
            }
        });
    });
}

void MoeTuner::estimate(int n, const std::function<void()> &done)
{
    if (m_params.fitExe.isEmpty() || m_estimates.contains(n)) {
        done();
        return;
    }
    m_status->setText(tr("Estimating GPU memory with %1 layers on the CPU...").arg(n));
    const QStringList args = m_params.fitArgs
                             + QStringList{QStringLiteral("-ncmoe"), QString::number(n), QStringLiteral("-fitp"),
                                           QStringLiteral("on")};
    runTool(m_params.fitExe, args, [this, n, done](bool ok, const QByteArray &out) {
        // One line per device: "<device> <model MiB> <context MiB> <compute MiB>", the last one "Host".
        Estimate e;
        bool freeKnown = true;
        for (const QByteArray &line : out.split('\n')) {
            const QList<QByteArray> f = line.simplified().split(' ');
            if (f.size() != 4 || f.at(0) == "Host")
                continue;
            const QString device = QString::fromUtf8(f.at(0));
            e.needMiB = qMax<qint64>(e.needMiB, 0) + f.at(1).toLongLong() + f.at(2).toLongLong() + f.at(3).toLongLong();
            if (m_freeMiB.contains(device))
                e.freeMiB = qMax<qint64>(e.freeMiB, 0) + m_freeMiB.value(device);
            else
                freeKnown = false;
        }
        if (!ok || e.needMiB < 0) {
            e = Estimate();
        } else {
            e.needMiB += m_params.extraGpuMiB;
            if (!freeKnown)
                e.freeMiB = -1;
            e.fits = e.freeMiB < 0 || e.needMiB <= e.freeMiB - kMarginMiB;
        }
        m_estimates.insert(n, e);
        done();
    });
}

void MoeTuner::findMinimum(int lo, int hi)
{
    // hi fits, values below lo do not; the memory needed grows as layers move to the GPU.
    if (lo >= hi) {
        m_minFitting = hi;
        if (hi > 0 && m_estimates.contains(hi - 1)) {
            // Show where the GPU memory ends.
            Result r;
            r.n = hi - 1;
            r.failed = true;
            r.noMemory = true;
            m_results << r;
            addRow(r);
        }
        startBenchmarks();
        return;
    }
    const int mid = (lo + hi) / 2;
    estimate(mid, [this, lo, hi, mid] {
        if (m_estimates.value(mid).fits)
            findMinimum(lo, mid);
        else
            findMinimum(mid + 1, hi);
    });
}

void MoeTuner::startBenchmarks()
{
    // From all layers on the CPU down to the fewest that fit, in about kTargetCoarseRuns steps.
    m_step = qMax(1, int(std::ceil(double(m_params.layers - m_minFitting) / kTargetCoarseRuns)));
    m_queue.clear();
    for (int n = m_params.layers; n > m_minFitting; n -= m_step)
        m_queue << n;
    m_queue << m_minFitting;
    m_phase = Phase::Coarse;
    runNext();
}

void MoeTuner::runNext()
{
    if (m_queue.isEmpty()) {
        const Result *b = best();
        if (m_phase == Phase::Coarse && b && m_step > 1) {
            // Fewer layers on the CPU is usually faster as long as they fit: try the values between
            // the best one and the next (lower) one that was tested.
            m_phase = Phase::Refine;
            m_slowerInARow = 0;
            for (int n = b->n - 1; n > b->n - m_step && n >= m_minFitting; --n) {
                if (!tested(n))
                    m_queue << n;
            }
            if (!m_queue.isEmpty()) {
                runNext();
                return;
            }
        }
        finish(QString());
        return;
    }
    runOne(m_queue.takeFirst());
}

void MoeTuner::runOne(int n)
{
    estimate(n, [this, n] {
        m_current = n;
        QStringList args{QStringLiteral("-m"), m_params.model, QStringLiteral("-ngl"), QStringLiteral("99"),
                         QStringLiteral("-ncmoe"), QString::number(n)};
        args << kBenchArgs << m_params.extraArgs;
        m_runClock.start();
        m_elapsedTimer->start();
        m_status->setText(tr("Testing %1 layers on the CPU...").arg(n));
        runTool(m_params.benchExe, args, [this](bool ok, const QByteArray &out) { onFinished(ok, out); });
    });
}

void MoeTuner::onFinished(bool ok, const QByteArray &out)
{
    m_elapsedTimer->stop();
    Result r;
    r.n = m_current;
    for (const QByteArray &line : out.split('\n')) {
        const QJsonObject o = QJsonDocument::fromJson(line).object();
        if (o.isEmpty())
            continue;
        const double tps = o.value(QLatin1String("avg_ts")).toDouble(-1);
        if (o.value(QLatin1String("n_gen")).toInt() > 0)
            r.genTps = tps;
        else if (o.value(QLatin1String("n_prompt")).toInt() > 0)
            r.promptTps = tps;
    }
    r.failed = !ok || r.genTps <= 0;

    const Result *b = best();
    const double bestTps = b ? b->genTps : 0;
    m_results << r;
    addRow(r);

    if (r.failed) {
        // Usually out of GPU memory: fewer layers on the CPU will not fit either.
        QStringList tail = m_stderrTail.trimmed().split(QLatin1Char('\n'));
        tail = tail.mid(qMax(0, int(tail.size()) - 2));
        m_status->setText(tr("%1 layers on the CPU failed: %2").arg(r.n).arg(tail.join(QLatin1Char(' ')).trimmed()));
        m_queue.clear();
    } else if (r.genTps < bestTps * kSlowerFactor) {
        if (++m_slowerInARow >= 2)
            m_queue.clear(); // past the peak
    } else {
        m_slowerInARow = 0;
    }
    runNext();
}

void MoeTuner::finish(const QString &message)
{
    m_phase = Phase::Done;
    m_current = -1;
    m_elapsedTimer->stop();
    m_progress->setRange(0, 1);
    m_progress->setValue(1);
    m_startButton->setEnabled(true);
    m_startButton->setText(tr("Run Again"));

    const Result *b = best();
    if (!message.isEmpty()) {
        m_status->setText(message);
    } else if (!b) {
        m_status->setText(tr("No run succeeded. See whether llama-bench works with this model and device."));
    } else {
        QString text = tr("Fastest: %1 layers on the CPU, %2 tok/s.").arg(b->n).arg(number(b->genTps, 1));
        if (!m_estimateNote.isEmpty())
            text += QLatin1Char(' ') + m_estimateNote
                    + QLatin1Char(' ') + tr("If the server gets slower than in this test, its context does not fit "
                                            "next to the experts: use a higher number.");
        else if (b->n == m_minFitting && m_minFitting > 0)
            text += QLatin1Char(' ') + tr("Moving more expert layers to the GPU would not leave room for the context.");
        m_status->setText(text);
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == b->n) {
                m_table->setCurrentCell(row, 0);
                break;
            }
        }
    }
}

void MoeTuner::addRow(const Result &r)
{
    // Sorted by the number of layers on the CPU, so the table reads as a curve.
    int row = 0;
    while (row < m_table->rowCount() && m_table->item(row, 0)->data(Qt::UserRole).toInt() > r.n)
        ++row;
    m_table->insertRow(row);
    auto *n = new QTableWidgetItem(QString::number(r.n));
    n->setData(Qt::UserRole, r.n);
    const Estimate e = m_estimates.value(r.n);
    QString memText = QStringLiteral("—");
    if (e.needMiB >= 0)
        memText = e.freeMiB >= 0 ? tr("%1 of %2").arg(e.needMiB).arg(e.freeMiB) : QString::number(e.needMiB);
    auto *mem = new QTableWidgetItem(memText);
    auto *pp = new QTableWidgetItem(r.promptTps > 0 ? number(r.promptTps, 1) : QStringLiteral("—"));
    auto *tg = new QTableWidgetItem(r.noMemory ? tr("does not fit") : r.failed ? tr("failed") : number(r.genTps, 2));
    tg->setData(Qt::UserRole, !r.failed);
    for (QTableWidgetItem *item : {n, mem, pp, tg})
        item->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, n);
    m_table->setItem(row, 1, mem);
    m_table->setItem(row, 2, pp);
    m_table->setItem(row, 3, tg);
    m_table->scrollToItem(n);
}

const MoeTuner::Result *MoeTuner::best() const
{
    const Result *b = nullptr;
    for (const Result &r : m_results) {
        if (!r.failed && (!b || r.genTps > b->genTps))
            b = &r;
    }
    return b;
}

bool MoeTuner::tested(int n) const
{
    for (const Result &r : m_results) {
        if (r.n == n)
            return true;
    }
    return false;
}
