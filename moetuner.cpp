#include "moetuner.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QProcess>
#include <QProgressBar>
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
                       "starting with all %1 on the CPU and moving them to the GPU until it runs out of memory "
                       "or generation gets slower. Each run loads the model, so this takes a few minutes.")
                        .arg(m_params.layers));

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Layers on CPU"), tr("Prompt, tok/s"), tr("Generation, tok/s")});
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
        const bool usable = row >= 0 && m_phase == Phase::Done && m_table->item(row, 2)->data(Qt::UserRole).toBool();
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
    m_table->setRowCount(0);
    m_useButton->setEnabled(false);
    m_useButton->setText(tr("Use"));
    m_startButton->setEnabled(false);
    m_progress->setRange(0, 0); // busy
    m_slowerInARow = 0;

    // From all layers on the CPU down to none, in about kTargetCoarseRuns steps.
    m_step = qMax(1, int(std::ceil(double(m_params.layers) / kTargetCoarseRuns)));
    m_queue.clear();
    for (int n = m_params.layers; n > 0; n -= m_step)
        m_queue << n;
    m_queue << 0;
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
            for (int n = b->n - 1; n > b->n - m_step && n >= 0; --n) {
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
    m_current = n;
    m_stderrTail.clear();
    QStringList args{QStringLiteral("-m"), m_params.model, QStringLiteral("-ngl"), QStringLiteral("99"),
                     QStringLiteral("-ncmoe"), QString::number(n)};
    args << kBenchArgs << m_params.extraArgs;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        // llama-bench logs to stderr; keep the end for the error message of a failed run.
        m_stderrTail += QString::fromUtf8(m_process->readAllStandardError());
        if (m_stderrTail.size() > 4000)
            m_stderrTail = m_stderrTail.right(4000);
    });
    connect(m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        onFinished(status == QProcess::NormalExit ? code : -1);
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_process->deleteLater();
            m_process = nullptr;
            finish(tr("Could not run %1.").arg(m_params.benchExe));
        }
    });
    m_runClock.start();
    m_elapsedTimer->start();
    m_status->setText(tr("Testing %1 layers on the CPU...").arg(n));
    m_process->start(m_params.benchExe, args);
}

void MoeTuner::onFinished(int exitCode)
{
    m_elapsedTimer->stop();
    Result r;
    r.n = m_current;
    const QList<QByteArray> lines = m_process->readAllStandardOutput().split('\n');
    for (const QByteArray &line : lines) {
        const QJsonObject o = QJsonDocument::fromJson(line).object();
        if (o.isEmpty())
            continue;
        const double tps = o.value(QLatin1String("avg_ts")).toDouble(-1);
        if (o.value(QLatin1String("n_gen")).toInt() > 0)
            r.genTps = tps;
        else if (o.value(QLatin1String("n_prompt")).toInt() > 0)
            r.promptTps = tps;
    }
    m_process->deleteLater();
    m_process = nullptr;
    r.failed = exitCode != 0 || r.genTps <= 0;

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
            m_queue.clear(); // past the peak (e.g. spilling into shared memory)
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
        m_status->setText(tr("Fastest: %1 layers on the CPU, %2 tok/s. The server also needs GPU memory for the "
                             "context (KV cache), which these short runs hardly use: if it does not load, "
                             "use a slightly higher number.")
                              .arg(b->n).arg(number(b->genTps, 1)));
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
    auto *pp = new QTableWidgetItem(r.promptTps > 0 ? number(r.promptTps, 1) : QStringLiteral("—"));
    auto *tg = new QTableWidgetItem(r.failed ? tr("failed") : number(r.genTps, 2));
    tg->setData(Qt::UserRole, !r.failed);
    for (QTableWidgetItem *item : {n, pp, tg})
        item->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, n);
    m_table->setItem(row, 1, pp);
    m_table->setItem(row, 2, tg);
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
