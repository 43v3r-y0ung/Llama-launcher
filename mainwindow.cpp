#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tlscert.h"

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QStandardItemModel>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStandardPaths>
#include <QStatusBar>
#include <QThread>
#include <QToolButton>

#include <cmath>
#include <limits>

#include <unistd.h>

namespace {

constexpr int kMaxRecentModels = 10;
constexpr int kMaxConsoleLines = 5000;
constexpr int kKillTimeoutMs = 5000;
constexpr int kDeviceFreeMiBRole = Qt::UserRole + 1;

enum MoeMode { MoeAuto = 0, MoeAll = 1, MoeFirstN = 2 };
enum FlashAttn { FaAuto = 0, FaOn = 1, FaOff = 2 };
enum Reasoning { ReasoningAuto = 0, ReasoningOn = 1, ReasoningOff = 2 };

// KV cache types offered in kvCacheCombo, by index; index 0 is the server default (f16).
const QStringList kKvCacheTypes = {QStringLiteral("f16"), QStringLiteral("q8_0"), QStringLiteral("q4_0")};

// Inverse of QProcess::splitCommand(): quote tokens that would otherwise be split or mangled.
// Server arguments grouped by option: "--temp 0.8" -> {"--temp", "0.8"}. Repeated options keep all values.
QMap<QString, QStringList> groupOptions(const QStringList &args)
{
    static const QRegularExpression number(QStringLiteral("^-?[0-9.]+$"));
    QMap<QString, QStringList> groups;
    QString option;
    QStringList values;
    auto flush = [&] {
        if (!option.isEmpty() || !values.isEmpty())
            groups[option] << values.join(QLatin1Char(' '));
    };
    for (const QString &a : args) {
        if (a.startsWith(QLatin1Char('-')) && !number.match(a).hasMatch()) {
            flush();
            option = a;
            values.clear();
        } else {
            values << (option == QLatin1String("--model") || option == QLatin1String("--mmproj") ? QFileInfo(a).fileName() : a);
        }
    }
    flush();
    return groups;
}

// What changes when the server is started with `next` instead of `running`, one line per option.
QStringList diffArguments(const QStringList &running, const QStringList &next)
{
    const QMap<QString, QStringList> before = groupOptions(running), after = groupOptions(next);
    auto show = [](const QString &option, const QStringList &values) {
        const QString v = values.join(QStringLiteral(", ")).trimmed();
        return v.isEmpty() ? option : option + QLatin1Char(' ') + v;
    };
    QStringList lines;
    for (auto it = before.begin(); it != before.end(); ++it) {
        if (!after.contains(it.key()))
            lines << QStringLiteral("\u2212 ") + show(it.key(), it.value());
        else if (after.value(it.key()) != it.value())
            lines << QStringLiteral("%1: %2 \u2192 %3").arg(it.key(), it.value().join(QStringLiteral(", ")),
                                                          after.value(it.key()).join(QStringLiteral(", ")));
    }
    for (auto it = after.begin(); it != after.end(); ++it) {
        if (!before.contains(it.key()))
            lines << QStringLiteral("+ ") + show(it.key(), it.value());
    }
    return lines;
}

QString joinCommand(const QStringList &tokens)
{
    QStringList quoted;
    for (const QString &t : tokens) {
        if (t.isEmpty() || t.contains(QLatin1Char(' ')) || t.contains(QLatin1Char('\t')) || t.contains(QLatin1Char('"'))) {
            QString escaped = t;
            escaped.replace(QLatin1String("\""), QLatin1String("\"\"\"")); // splitCommand: """ is a literal quote
            quoted << QLatin1Char('"') + escaped + QLatin1Char('"');
        } else {
            quoted << t;
        }
    }
    return quoted.join(QLatin1Char(' '));
}

// The multimodal projector shipped next to a model (mmproj*.gguf); the one whose name
// shares the longest prefix with the model wins when there are several.
QString findMmproj(const QString &modelPath)
{
    const QFileInfo model(modelPath);
    const QFileInfoList candidates = model.dir().entryInfoList({QStringLiteral("*mmproj*.gguf")}, QDir::Files, QDir::Name);
    QString best;
    int bestScore = -1;
    for (const QFileInfo &fi : candidates) {
        const QString a = fi.fileName().toLower(), b = model.fileName().toLower();
        int common = 0;
        while (common < a.size() && common < b.size() && a.at(common) == b.at(common))
            ++common;
        if (common > bestScore) {
            bestScore = common;
            best = fi.absoluteFilePath();
        }
    }
    return best;
}

QString modelSettingsGroup(const QString &modelPath)
{
    // Paths contain '/', which QSettings would treat as nested groups, so key by a hash.
    const QByteArray hash = QCryptographicHash::hash(modelPath.toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    return QStringLiteral("models/") + QString::fromLatin1(hash);
}

QString processErrorText(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        return MainWindow::tr("the program could not be started (file not found or not executable)");
    case QProcess::Crashed:
        return MainWindow::tr("the process crashed");
    case QProcess::Timedout:
        return MainWindow::tr("operation timed out");
    case QProcess::WriteError:
        return MainWindow::tr("write error");
    case QProcess::ReadError:
        return MainWindow::tr("read error");
    default:
        return MainWindow::tr("unknown error");
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_server(new ServerProcess(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_healthTimer(new QTimer(this))
    , m_killTimer(new QTimer(this))
    , m_monitor(new Monitor(this))
{
    ui->setupUi(this);

    ui->contextSizeCombo->setValidator(new QIntValidator(1, 10000000, this));
    ui->consoleOutput->setMaximumBlockCount(kMaxConsoleLines);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->consoleOutput->setFont(mono);
    ui->logAdviceLabel->setVisible(false); // shown once the server log produces a hint
    connect(ui->logAdviceLabel, &QLabel::linkActivated, this, &MainWindow::onLogAdviceLinkActivated);
    ui->openBrowserButton->setIcon(QIcon::fromTheme(QStringLiteral("internet-web-browser")));
    ui->copyUrlButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    ui->refreshDevicesButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    ui->copyLogButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    ui->clearLogButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    setupCornerControls();

    ui->threadsSpin->setValue(qMax(1, QThread::idealThreadCount() / 2));
    for (QCheckBox *check : {ui->gpuAutoCheck, ui->threadsAutoCheck, ui->batchAutoCheck, ui->parallelAutoCheck,
                             ui->reasoningUnlimitedCheck, ui->mmprojCheck, ui->tempCheck, ui->topPCheck,
                             ui->topKCheck, ui->minPCheck, ui->presencePenaltyCheck, ui->frequencyPenaltyCheck})
        connect(check, &QCheckBox::toggled, this, &MainWindow::updateUiState);
    for (QComboBox *combo : {ui->reasoningCombo, ui->kvCacheCombo, ui->flashAttnCombo})
        connect(combo, &QComboBox::currentIndexChanged, this, &MainWindow::updateUiState);
    // The physical batch cannot exceed the logical one.
    connect(ui->batchSpin, &QSpinBox::valueChanged, ui->ubatchSpin, &QSpinBox::setMaximum);
    ui->ubatchSpin->setMaximum(ui->batchSpin->value());
    connect(ui->extraArgsEdit, &QLineEdit::editingFinished, this, [this] { migrateExtraArgs(); });
    connect(ui->contextAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);
    connect(ui->moeModeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateUiState);
    connect(ui->moeModeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateFitWarning);
    connect(ui->deviceCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateFitWarning);
    connect(ui->modelCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onModelChanged);
    connect(ui->serverPathEdit, &QLineEdit::editingFinished, this, &MainWindow::refreshServerInfo);
    connect(ui->modelsDirEdit, &QLineEdit::editingFinished, this, [this] { rescanModels(); });
    connect(ui->launchAtLoginCheck, &QCheckBox::toggled, this, &MainWindow::onLaunchAtLoginToggled);
    connect(ui->lanAccessCheck, &QCheckBox::toggled, this, &MainWindow::updateNetworkHint);
    connect(ui->apiKeyEdit, &QLineEdit::textChanged, this, &MainWindow::updateNetworkHint);
    connect(ui->portSpin, &QSpinBox::valueChanged, this, &MainWindow::updateNetworkHint);
    connect(ui->httpsCheck, &QCheckBox::toggled, this, &MainWindow::updateNetworkHint);
    connect(ui->httpsCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);
    // Traffic that leaves this computer is always encrypted.
    connect(ui->lanAccessCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (on)
            ui->httpsCheck->setChecked(true);
        updateUiState();
    });
    for (QLineEdit *edit : {ui->certFileEdit, ui->keyFileEdit}) {
        connect(edit, &QLineEdit::textChanged, this, &MainWindow::updateNetworkHint);
        connect(edit, &QLineEdit::textChanged, this, &MainWindow::updateUiState);
    }
    ui->networkHintLabel->setOpenExternalLinks(true);

    // The server runs on its own (see ServerProcess): the launcher can be restarted or upgraded
    // while it serves requests, and picks it up again when it starts.
    m_launcherPath = QCoreApplication::applicationFilePath();
    connect(m_server, &ServerProcess::outputLines, this, &MainWindow::handleServerOutput);
    connect(m_server, &ServerProcess::serverPidKnown, m_monitor, &Monitor::setProcessId);
    connect(m_server, &ServerProcess::finished, this, &MainWindow::handleServerFinished);

    connect(m_healthTimer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    setupCharts();
    setupSamplingPresets();
    connect(m_monitor, &Monitor::sampled, this, &MainWindow::onMonitorSample);
    m_healthTimer->setInterval(2000);

    m_killTimer->setSingleShot(true);
    m_killTimer->setInterval(kKillTimeoutMs);
    connect(m_killTimer, &QTimer::timeout, this, [this] {
        if (m_server->isRunning()) {
            ui->consoleOutput->appendPlainText(tr("Server did not stop in time, killing it."));
            m_server->kill();
        }
    });

    setupSystemTray();
    loadSettings();
    setStatus(Status::Stopped);
    refreshServerInfo();

    if (!adoptRunningServer() && ui->autoStartServerCheck->isChecked()) {
        QTimer::singleShot(0, this, [this] {
            if (!m_serverAutostartCancelled)
                startServer();
        });
    }
}

MainWindow::~MainWindow()
{
    saveSettings();

    // Probe processes (--version/--list-devices) emit finished() when they are destroyed;
    // make sure none of that reaches handlers that use `ui`. The server itself is left alone:
    // quitApplication() stops it, otherwise the next launcher adopts it.
    const auto processes = findChildren<QProcess *>();
    for (QProcess *process : processes)
        process->disconnect(this);
    m_server->disconnect(this);
    delete ui;
}

// ---------------------------------------------------------------------------
// Window & tray
// ---------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_quitting) {
        event->accept();
        return;
    }
    if (hasTray()) {
        saveSettings();
        hide();
        event->ignore();
    } else {
        quitApplication();
        event->accept();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateMonitorActivity();
}

void MainWindow::hideEvent(QHideEvent *event)
{
    QMainWindow::hideEvent(event);
    updateMonitorActivity();
}

// ---------------------------------------------------------------------------
// Monitoring
// ---------------------------------------------------------------------------

namespace {

constexpr double kGiB = 1024.0 * 1024.0 * 1024.0;

double orNan(double v)
{
    return v < 0 ? std::numeric_limits<double>::quiet_NaN() : v;
}

double bytesToGiB(qint64 bytes)
{
    return bytes < 0 ? std::numeric_limits<double>::quiet_NaN() : bytes / kGiB;
}

QString formatNumber(double v, int decimals)
{
    return QLocale().toString(v, 'f', decimals);
}

} // namespace

void MainWindow::setupCharts()
{
    auto percent = [](double v) { return QStringLiteral("%1 %").arg(formatNumber(v, 0)); };
    auto gib = [](double v) { return MainWindow::tr("%1 GiB").arg(formatNumber(v, 1)); };
    auto tps = [](double v) { return MainWindow::tr("%1 tok/s").arg(formatNumber(v, 1)); };

    ui->cpuChart->setTitle(tr("CPU"));
    ui->cpuChart->setSeriesNames({tr("System"), QStringLiteral("llama-server")});
    ui->cpuChart->setFormatter(percent);
    ui->cpuChart->setMaximum(100);

    ui->ramChart->setTitle(tr("RAM"));
    ui->ramChart->setSeriesNames({tr("System"), QStringLiteral("llama-server")});
    ui->ramChart->setFormatter(gib);

    ui->gpuChart->setTitle(tr("GPU load"));
    ui->gpuChart->setFormatter(percent);
    ui->gpuChart->setMaximum(100);

    ui->vramChart->setTitle(tr("VRAM"));
    ui->vramChart->setFormatter(gib);

    ui->tpsChart->setTitle(tr("Generation speed"));
    ui->tpsChart->setFormatter(tps);

    for (ChartWidget *chart : {ui->cpuChart, ui->ramChart, ui->gpuChart, ui->vramChart, ui->tpsChart})
        chart->setCapacity(Monitor::kHistorySize);

    ui->serverStatsLabel->setText(tr("The server is not running."));
}

void MainWindow::updateMonitorActivity()
{
    // In the tray with the server stopped there is nothing to show or record.
    const bool wanted = isVisible() || m_server->isRunning();
    if (wanted && !m_monitor->isActive())
        m_monitor->start();
    else if (!wanted && m_monitor->isActive())
        m_monitor->stop();
}

void MainWindow::onMonitorSample(const MonitorSample &s)
{
    const QList<MonitorSample> &history = m_monitor->history();
    QList<double> cpu, procCpu, ram, rss, gpu, vram, gen;
    for (const MonitorSample &h : history) {
        cpu << orNan(h.cpuPercent);
        procCpu << orNan(h.procCpuPercent);
        ram << bytesToGiB(h.ramUsed);
        rss << bytesToGiB(h.procRss);
        gpu << orNan(h.gpuBusy);
        vram << bytesToGiB(h.vramUsed);
        gen << orNan(h.genTps);
    }

    ui->cpuChart->setData({cpu, procCpu});
    ui->ramChart->setData({ram, rss});
    if (s.ramTotal > 0)
        ui->ramChart->setMaximum(s.ramTotal / kGiB);

    // AMD GPU
    ui->gpuChart->setVisible(s.gpuPresent);
    ui->vramChart->setVisible(s.gpuPresent);
    if (s.gpuPresent) {
        const QString name = m_gpuName.isEmpty() ? m_monitor->gpuName() : m_gpuName;
        ui->gpuChart->setToolTip(name);
        ui->vramChart->setToolTip(name);
        ui->gpuChart->setData({gpu});
        QStringList sensors;
        if (s.gpuSuspended) {
            sensors << tr("sleeping (power saving)");
        } else {
            if (s.gpuTempC >= 0)
                sensors << QStringLiteral("%1 °C").arg(formatNumber(s.gpuTempC, 0));
            if (s.gpuPowerW >= 0)
                sensors << tr("%1 W").arg(formatNumber(s.gpuPowerW, 0));
            if (s.gpuClockMHz >= 0)
                sensors << tr("%1 MHz").arg(s.gpuClockMHz);
        }
        ui->gpuChart->setSubtitle(sensors.join(QStringLiteral(" · ")));
        ui->vramChart->setData({vram});
        if (s.vramTotal > 0)
            ui->vramChart->setMaximum(s.vramTotal / kGiB);
        ui->vramChart->setSubtitle(s.gttUsed >= 0
            ? tr("Shared system memory (GTT): %1 GiB").arg(formatNumber(s.gttUsed / kGiB, 2))
            : QString());
    }

    // llama-server
    const bool running = m_server->isRunning();
    ui->tpsChart->setData({gen});
    QStringList last;
    if (s.lastGenTps >= 0)
        last << tr("generation %1 tok/s").arg(formatNumber(s.lastGenTps, 2));
    if (s.lastPromptTps >= 0)
        last << tr("prompt %1 tok/s").arg(formatNumber(s.lastPromptTps, 1));
    ui->tpsChart->setSubtitle(last.isEmpty() ? QString() : tr("Last request: %1").arg(last.join(QStringLiteral(", "))));

    if (!running) {
        ui->serverStatsLabel->setText(tr("The server is not running."));
        return;
    }
    if (s.busySlots != m_busySlots) {
        m_busySlots = s.busySlots;
        updateStatusText();
    }
    if (m_whenIdle != IdleNothing && m_status == Status::Running) {
        // Two idle readings in a row, so a request that is just being handed over is not cut off.
        // Without /slots (busySlots -1) nobody can tell, so keep waiting.
        const bool idle = s.busySlots == 0 && !s.busy;
        m_idleSamples = idle ? m_idleSamples + 1 : 0;
        if (m_idleSamples >= 2) {
            const WhenIdle what = m_whenIdle;
            m_whenIdle = IdleNothing;
            m_idleSamples = 0;
            ui->consoleOutput->appendPlainText(tr("The server is idle now."));
            if (what == IdleRestart)
                restartServer();
            else
                stopServer();
            updateStatusText();
        }
    }
    QStringList parts;
    if (s.promptProgress >= 0)
        parts << tr("Reading prompt: <b>%1 %</b> at %2 tok/s")
                     .arg(formatNumber(s.promptProgress * 100, 0), formatNumber(s.promptTps, 1));
    else if (s.busy)
        parts << tr("Generating: <b>%1 tok/s</b>").arg(formatNumber(qMax(0.0, s.genTps), 1));
    else
        parts << (m_status == Status::Running ? tr("Idle") : tr("Loading model"));
    if (s.ctxSize > 0 && s.ctxUsed >= 0)
        parts << tr("Context: %1 / %2 (%3 %)")
                     .arg(s.ctxUsed).arg(s.ctxSize).arg(formatNumber(100.0 * s.ctxUsed / s.ctxSize, 0));
    if (s.procRss >= 0)
        parts << tr("llama-server uses %1 GiB RAM").arg(formatNumber(s.procRss / kGiB, 1));
    ui->serverStatsLabel->setText(parts.join(QStringLiteral(" &middot; ")));
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_trayMenu = new QMenu(this);
    m_startAction = m_trayMenu->addAction(tr("Start Server"), this, &MainWindow::startServer);
    m_stopAction = m_trayMenu->addAction(tr("Stop Server"), this, &MainWindow::stopServer);
    m_openAction = m_trayMenu->addAction(tr("Open in Browser"), this, &MainWindow::on_openBrowserButton_clicked);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(tr("Show/Restore"), this, &MainWindow::toggleWindowVisibility);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(tr("Exit"), this, &MainWindow::requestQuit);

    m_trayIcon = new QSystemTrayIcon(QIcon(QStringLiteral(":/rsc/llama-launcher.svg")), this);
    m_trayIcon->setToolTip(tr("Llama Launcher"));
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this] {
        show();
        showNormal();
        raise();
        activateWindow();
    });
    m_trayIcon->show();
}

void MainWindow::toggleWindowVisibility()
{
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        bringToFront();
    }
}

void MainWindow::bringToFront()
{
    show();
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        toggleWindowVisibility();
}

// Start/Stop and the window menu sit at the right end of the tab bar: starting and stopping
// is occasional, so it does not deserve a full-width button.
void MainWindow::setupCornerControls()
{
    m_startStopButton = new QToolButton(this);
    m_startStopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_startStopButton, &QToolButton::clicked, this, &MainWindow::toggleServer);

    auto *menu = new QMenu(this);
    m_menuStartAction = menu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), tr("Start Server"),
                                        this, &MainWindow::startServer);
    m_menuStopAction = menu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), tr("Stop Server"),
                                       this, [this] { m_restartPending = false; stopServer(); });
    m_menuRestartAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Restart Server"),
                                          this, &MainWindow::restartServer);
    // Wait for the requests in progress to finish first; choosing it again cancels it.
    m_restartIdleAction = menu->addAction(tr("Restart Server When Idle"), this, [this] { setWhenIdle(IdleRestart); });
    m_restartIdleAction->setCheckable(true);
    m_stopIdleAction = menu->addAction(tr("Stop Server When Idle"), this, [this] { setWhenIdle(IdleStop); });
    m_stopIdleAction->setCheckable(true);
    menu->addSeparator();
    menu->addAction(QIcon::fromTheme(QStringLiteral("system-reboot")), tr("Restart Launcher (Server Keeps Running)"),
                    this, &MainWindow::restartLauncher);
    QAction *quit = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit"),
                                    this, &MainWindow::requestQuit);
    quit->setShortcut(QKeySequence::Quit);
    addAction(quit); // Ctrl+Q works without opening the menu

    auto *menuButton = new QToolButton(this);
    menuButton->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    if (menuButton->icon().isNull())
        menuButton->setText(QStringLiteral("\u2630"));
    menuButton->setToolTip(tr("Menu"));
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; }"));
    menuButton->setMenu(menu);

    // Shown while the settings differ from what the running server was started with.
    m_applyButton = new QToolButton(this);
    m_applyButton->setText(tr("Restart to Apply"));
    m_applyButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_applyButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_applyButton->setPopupMode(QToolButton::InstantPopup);
    auto *applyMenu = new QMenu(m_applyButton);
    applyMenu->addAction(m_menuRestartAction);
    applyMenu->addAction(m_restartIdleAction);
    m_applyButton->setMenu(applyMenu);
    m_applyButton->hide();
    m_pendingTimer = new QTimer(this);
    m_pendingTimer->setInterval(1000);
    connect(m_pendingTimer, &QTimer::timeout, this, &MainWindow::updatePendingChanges);
    m_pendingTimer->start();

    auto *corner = new QWidget(this);
    auto *layout = new QHBoxLayout(corner);
    layout->setContentsMargins(0, 0, 0, 2);
    layout->setSpacing(4);
    layout->addWidget(m_applyButton);
    layout->addWidget(m_startStopButton);
    layout->addWidget(menuButton);
    ui->tabWidget->setCornerWidget(corner, Qt::TopRightCorner);
}

void MainWindow::requestQuit()
{
    if (!m_server->isRunning()) {
        quitApplication();
        return;
    }
    if (isHidden())
        bringToFront(); // the question must not pop up out of nowhere over other windows

    const QString state = m_busySlots > 0 ? tr("The server is processing %n request(s).", nullptr, m_busySlots)
                                          : tr("The server is running.");
    QMessageBox box(QMessageBox::Question, tr("Quit Llama Launcher"), state, QMessageBox::NoButton, this);
    box.setInformativeText(tr("Stop it, or keep it running: the launcher picks it up again the next time it starts."));
    QPushButton *keep = box.addButton(tr("Keep Server Running"), QMessageBox::AcceptRole);
    QPushButton *stop = box.addButton(tr("Stop Server and Quit"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(keep);
    box.exec();
    if (box.clickedButton() == keep)
        quitKeepingServer();
    else if (box.clickedButton() == stop)
        quitApplication();
}

void MainWindow::quitApplication()
{
    m_quitting = true;
    saveSettings();
    if (m_server->isRunning()) {
        m_stopRequested = true;
        m_server->stopAndWait(3000);
    }
    qApp->quit();
}

void MainWindow::quitKeepingServer()
{
    m_quitting = true;
    saveSettings();
    qApp->quit();
}

void MainWindow::restartLauncher()
{
    saveSettings();
    QString path = m_launcherPath;
    path.remove(QStringLiteral(" (deleted)")); // replaced by a new build since it was started
    QStringList args{QStringLiteral("--replace"), QString::number(QCoreApplication::applicationPid())};
    if (isHidden())
        args << QStringLiteral("--autostart"); // stay in the tray, as now
    if (!QProcess::startDetached(path, args)) {
        QMessageBox::warning(this, tr("Warning"), tr("Could not start %1.").arg(path));
        return;
    }
    m_quitting = true;
    qApp->quit();
}

void MainWindow::notify(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon)
{
    if (m_trayIcon && ui->notificationsCheck->isChecked())
        m_trayIcon->showMessage(title, message, icon, 8000);
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

QUrl MainWindow::serverUrl() const
{
    return QUrl(QStringLiteral("%1://127.0.0.1:%2/").arg(m_runningHttps ? "https" : "http").arg(m_runningPort));
}

QString MainWindow::lanAddress()
{
    return lanAddresses().value(0);
}

QStringList MainWindow::lanAddresses()
{
    // IPv4 addresses of real network cards; skip loopback and virtual bridges
    // (Docker, libvirt, VPN tunnels...) whose addresses are useless to other machines.
    QStringList result;
    static const QRegularExpression virtualIf(QStringLiteral("^(docker|br-|veth|virbr|vmnet|vboxnet|tun|tap|wg|lxc|lxd|podman|cni|flannel|zt)"));
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
            || (flags & QNetworkInterface::IsLoopBack) || virtualIf.match(iface.name()).hasMatch())
            continue;
        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback() && !ip.isLinkLocal()
                && !result.contains(ip.toString()))
                result << ip.toString();
        }
    }
    return result;
}

QUrl MainWindow::lanUrl() const
{
    const QString ip = m_runningLan ? lanAddress() : QString();
    return ip.isEmpty() ? QUrl()
                        : QUrl(QStringLiteral("%1://%2:%3/").arg(m_runningHttps ? "https" : "http", ip).arg(m_runningPort));
}

void MainWindow::setStatus(Status status)
{
    const Status previous = m_status;
    m_status = status;
    const bool running = m_server->isRunning();
    if (!running) {
        m_whenIdle = IdleNothing; // nothing left to wait for
        m_busySlots = -1;
    }
    updateStatusText();

    const bool ready = status == Status::Running;
    if (ready && previous != Status::Running)
        m_monitor->setServer(serverUrl(), m_server->info().apiKey, m_runningHttps ? m_runningCert : QSslCertificate());
    else if (status == Status::Stopped) {
        m_monitor->setServer(QUrl(), QString());
        m_monitor->setProcessId(0);
    }
    updateMonitorActivity();
    ui->openBrowserButton->setEnabled(ready && m_runningWebUi);
    ui->copyUrlButton->setEnabled(status != Status::Stopped);
    m_startStopButton->setText(running ? tr("Stop Server") : tr("Start Server"));
    m_startStopButton->setIcon(QIcon::fromTheme(running ? QStringLiteral("media-playback-stop")
                                                        : QStringLiteral("media-playback-start")));
    m_menuStartAction->setEnabled(!running);
    m_menuStopAction->setEnabled(running);
    m_menuRestartAction->setEnabled(running);
    m_restartIdleAction->setEnabled(running);
    m_stopIdleAction->setEnabled(running);

    if (m_trayIcon) {
        m_startAction->setEnabled(!running);
        m_stopAction->setEnabled(running);
        m_openAction->setEnabled(ready && m_runningWebUi);
    }

    if (previous != Status::Running && status == Status::Running) {
        const QUrl lan = lanUrl();
        notify(tr("Llama server is ready"),
               tr("%1\n%2").arg(m_runningModelName, (lan.isEmpty() ? serverUrl() : lan).toString()));
    }

    updateUiState();
    updatePendingChanges();
}

void MainWindow::updateStatusText()
{
    QString text, color;
    switch (m_status) {
    case Status::Stopped:  text = tr("Stopped");  color = QStringLiteral("#7f8c8d"); break;
    case Status::Starting: text = tr("Starting"); color = QStringLiteral("#e67e22"); break;
    case Status::Loading:  text = tr("Loading model"); color = QStringLiteral("#e67e22"); break;
    case Status::Running:  text = tr("Running");  color = QStringLiteral("#27ae60"); break;
    }
    QStringList notes; // shown after the status in both the window and the tray tooltip
    if (m_status == Status::Running && m_busySlots > 0)
        notes << tr("%n request(s) in progress", nullptr, m_busySlots);
    if (m_whenIdle == IdleRestart)
        notes << tr("restart when idle");
    else if (m_whenIdle == IdleStop)
        notes << tr("stop when idle");

    QString html = QStringLiteral("<span style='color:%1'>&#9679;</span> <b>%2</b>").arg(color, text);
    if (!notes.isEmpty())
        html += QStringLiteral(" (%1)").arg(notes.join(QStringLiteral(", ")).toHtmlEscaped());
    if (m_status != Status::Stopped) {
        const QString url = serverUrl().toString();
        // Links must not wrap in the middle when the status line wraps.
        html += QStringLiteral(" &mdash; %1 &mdash; <span style='white-space:nowrap'><a href='%2'>%2</a></span>")
                    .arg(m_runningModelName.toHtmlEscaped(), url.toHtmlEscaped());
        const QString lan = lanUrl().toString();
        if (!lan.isEmpty())
            html += QStringLiteral(" &middot; %1 <span style='white-space:nowrap'><a href='%2'>%2</a></span>")
                        .arg(tr("network:"), lan.toHtmlEscaped());
    }
    ui->statusLabel->setText(html);
    ui->statusLabel->setOpenExternalLinks(true);

    m_restartIdleAction->setChecked(m_whenIdle == IdleRestart);
    m_stopIdleAction->setChecked(m_whenIdle == IdleStop);

    if (m_trayIcon) {
        QString tooltip = tr("Llama Launcher\nStatus: %1").arg(text);
        if (!notes.isEmpty())
            tooltip += QStringLiteral(" (%1)").arg(notes.join(QStringLiteral(", ")));
        if (m_status != Status::Stopped)
            tooltip += tr("\nModel: %1\n%2").arg(m_runningModelName, serverUrl().toString());
        if (m_status != Status::Stopped && !lanUrl().isEmpty())
            tooltip += QLatin1Char('\n') + lanUrl().toString();
        m_trayIcon->setToolTip(tooltip);
    }
}

// ---------------------------------------------------------------------------
// Sampling presets: named sets of the "Default sampling" values, shared by all models
// ---------------------------------------------------------------------------

namespace {

struct SamplingField
{
    QCheckBox *check;
    QAbstractSpinBox *spin;
    const char *key; // settings key of the checkbox; the value is stored as <key>Value
};

double spinValue(QAbstractSpinBox *spin)
{
    if (auto *d = qobject_cast<QDoubleSpinBox *>(spin))
        return d->value();
    return qobject_cast<QSpinBox *>(spin)->value();
}

void setSpinValue(QAbstractSpinBox *spin, double v)
{
    if (auto *d = qobject_cast<QDoubleSpinBox *>(spin))
        d->setValue(v);
    else
        qobject_cast<QSpinBox *>(spin)->setValue(qRound(v));
}

const QString kPresetGroup = QStringLiteral("samplingPresets");

} // namespace

static QList<SamplingField> samplingFields(Ui::MainWindow *ui)
{
    return {
        {ui->tempCheck, ui->tempSpin, "temp"},
        {ui->topPCheck, ui->topPSpin, "topP"},
        {ui->topKCheck, ui->topKSpin, "topK"},
        {ui->minPCheck, ui->minPSpin, "minP"},
        {ui->presencePenaltyCheck, ui->presencePenaltySpin, "presencePenalty"},
        {ui->frequencyPenaltyCheck, ui->frequencyPenaltySpin, "frequencyPenalty"},
    };
}

void MainWindow::setupSamplingPresets()
{
    auto *row = new QHBoxLayout;
    auto *label = new QLabel(tr("Preset:"), this);
    m_presetCombo = new QComboBox(this);
    m_presetCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_presetCombo->setToolTip(tr("Named sets of these values, shared by all models. "
                                 "Shows the preset the current values match."));
    label->setBuddy(m_presetCombo);
    auto *save = new QToolButton(this);
    save->setText(tr("Save As..."));
    save->setToolTip(tr("Save the current values as a preset"));
    m_deletePresetButton = new QToolButton(this);
    m_deletePresetButton->setText(tr("Delete"));
    m_deletePresetButton->setToolTip(tr("Delete the selected preset"));
    row->addWidget(label);
    row->addWidget(m_presetCombo, 1);
    row->addWidget(save);
    row->addWidget(m_deletePresetButton);
    ui->samplingGrid->addLayout(row, ui->samplingGrid->rowCount(), 0, 1, ui->samplingGrid->columnCount());

    connect(m_presetCombo, &QComboBox::activated, this, &MainWindow::applySamplingPreset);
    connect(save, &QToolButton::clicked, this, &MainWindow::saveSamplingPreset);
    connect(m_deletePresetButton, &QToolButton::clicked, this, &MainWindow::deleteSamplingPreset);
    for (const SamplingField &f : samplingFields(ui)) {
        connect(f.check, &QCheckBox::toggled, this, &MainWindow::updatePresetCombo);
        if (auto *d = qobject_cast<QDoubleSpinBox *>(f.spin))
            connect(d, &QDoubleSpinBox::valueChanged, this, &MainWindow::updatePresetCombo);
        else
            connect(qobject_cast<QSpinBox *>(f.spin), &QSpinBox::valueChanged, this, &MainWindow::updatePresetCombo);
    }
    updatePresetCombo();
}

void MainWindow::updatePresetCombo()
{
    // Item data: preset name; empty for the built-in "Server defaults"; invalid for "Custom".
    QSettings settings;
    settings.beginGroup(kPresetGroup);
    QStringList names = settings.childGroups();
    names.sort(Qt::CaseInsensitive);

    const QList<SamplingField> fields = samplingFields(ui);
    auto matches = [&](const QString &name) {
        for (const SamplingField &f : fields) {
            const bool on = name.isEmpty() ? false : settings.value(name + QLatin1Char('/') + QLatin1String(f.key)).toBool();
            if (f.check->isChecked() != on)
                return false;
            if (on && qAbs(spinValue(f.spin) - settings.value(name + QLatin1Char('/') + QLatin1String(f.key) + QLatin1String("Value")).toDouble()) > 1e-6)
                return false;
        }
        return true;
    };

    const QSignalBlocker blocker(m_presetCombo);
    m_presetCombo->clear();
    m_presetCombo->addItem(tr("Custom"), QVariant());
    m_presetCombo->addItem(tr("Server defaults (nothing set)"), QString());
    for (const QString &name : std::as_const(names))
        m_presetCombo->addItem(name, name);

    int current = 0;
    if (matches(QString()))
        current = 1;
    for (int i = 0; i < names.size() && current == 0; ++i) {
        if (matches(names.at(i)))
            current = i + 2;
    }
    m_presetCombo->setCurrentIndex(current);
    // "Custom" is only a state, not something to pick.
    if (auto *model = qobject_cast<QStandardItemModel *>(m_presetCombo->model()))
        model->item(0)->setEnabled(false);
    m_deletePresetButton->setEnabled(current >= 2);
}

void MainWindow::applySamplingPreset(int index)
{
    const QVariant data = m_presetCombo->itemData(index);
    if (!data.isValid())
        return;
    const QString name = data.toString();
    QSettings settings;
    settings.beginGroup(kPresetGroup);
    for (const SamplingField &f : samplingFields(ui)) {
        const QString key = name + QLatin1Char('/') + QLatin1String(f.key);
        const bool on = !name.isEmpty() && settings.value(key).toBool();
        f.check->setChecked(on);
        if (on)
            setSpinValue(f.spin, settings.value(key + QLatin1String("Value")).toDouble());
    }
    updatePresetCombo();
    statusBar()->showMessage(tr("Applied the sampling preset \"%1\"").arg(m_presetCombo->currentText()), 3000);
}

void MainWindow::saveSamplingPreset()
{
    const QVariant current = m_presetCombo->currentData();
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save Sampling Preset"), tr("Preset name:"), QLineEdit::Normal,
                                               current.isValid() ? current.toString() : QString(), &ok).trimmed();
    if (!ok || name.isEmpty())
        return;
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) {
        QMessageBox::warning(this, tr("Warning"), tr("A preset name cannot contain / or \\."));
        return;
    }
    QSettings settings;
    settings.beginGroup(kPresetGroup);
    if (settings.childGroups().contains(name)
        && QMessageBox::question(this, tr("Save Sampling Preset"), tr("Replace the preset \"%1\"?").arg(name))
               != QMessageBox::Yes)
        return;
    settings.remove(name);
    settings.beginGroup(name);
    for (const SamplingField &f : samplingFields(ui)) {
        settings.setValue(QLatin1String(f.key), f.check->isChecked());
        settings.setValue(QLatin1String(f.key) + QLatin1String("Value"), spinValue(f.spin));
    }
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    updatePresetCombo();
}

void MainWindow::deleteSamplingPreset()
{
    const QString name = m_presetCombo->currentData().toString();
    if (name.isEmpty()
        || QMessageBox::question(this, tr("Delete Sampling Preset"), tr("Delete the preset \"%1\"?").arg(name))
               != QMessageBox::Yes)
        return;
    QSettings settings;
    settings.beginGroup(kPresetGroup);
    settings.remove(name);
    settings.endGroup();
    settings.sync();
    updatePresetCombo();
}

void MainWindow::updatePendingChanges()
{
    if (!m_server->isRunning()) {
        m_applyButton->hide();
        return;
    }
    if (!isVisible() || m_loading)
        return;

    const ServerProcess::Info &running = m_server->info();
    QStringList changes;
    const QString exe = serverExecutable();
    const QString resolved = QFileInfo(exe.contains(QLatin1Char('/')) ? exe : QStandardPaths::findExecutable(exe)).absoluteFilePath();
    if (resolved != running.exe)
        changes << tr("llama-server: %1 \u2192 %2").arg(running.exe, resolved);
    if (ui->apiKeyEdit->text().trimmed() != running.apiKey)
        changes << tr("API key changed");
    changes << diffArguments(running.args, buildArguments());

    m_applyButton->setVisible(!changes.isEmpty());
    if (!changes.isEmpty())
        m_applyButton->setToolTip(tr("Settings changed since the server was started:") + QLatin1Char('\n')
                                  + changes.join(QLatin1Char('\n')) + QStringLiteral("\n\n")
                                  + tr("They take effect when the server is restarted."));
}

void MainWindow::setWhenIdle(int action)
{
    if (!m_server->isRunning())
        return;
    // Choosing the pending action again cancels it.
    m_whenIdle = m_whenIdle == action ? IdleNothing : WhenIdle(action);
    m_idleSamples = 0;
    if (m_whenIdle != IdleNothing && m_status != Status::Running) {
        // Still loading the model: no request can be in progress.
        const WhenIdle what = m_whenIdle;
        m_whenIdle = IdleNothing;
        if (what == IdleRestart)
            restartServer();
        else
            stopServer();
    }
    updateStatusText();
}

void MainWindow::on_openBrowserButton_clicked()
{
    QDesktopServices::openUrl(serverUrl());
}

void MainWindow::on_copyUrlButton_clicked()
{
    // With network access on, the network address is the one worth sharing with other machines.
    const QUrl lan = lanUrl();
    const QString url = (lan.isEmpty() ? serverUrl() : lan).toString();
    QGuiApplication::clipboard()->setText(url);
    statusBar()->showMessage(tr("Copied %1").arg(url), 3000);
}

void MainWindow::on_generateKeyButton_clicked()
{
    // 128 random bits from the OS CSPRNG, hex-encoded.
    QByteArray bytes(16, Qt::Uninitialized);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(bytes.data()), bytes.size() / 4);
    ui->apiKeyEdit->setText(QString::fromLatin1(bytes.toHex()));
    ui->apiKeyEdit->setEchoMode(QLineEdit::Normal); // show it once so it can be copied
    ui->apiKeyEdit->selectAll();
    ui->apiKeyEdit->setFocus();
}

void MainWindow::updateNetworkHint()
{
    const bool lan = ui->lanAccessCheck->isChecked();
    const bool hasKey = !ui->apiKeyEdit->text().trimmed().isEmpty();
    const bool https = httpsEnabled();
    const auto warning = [](const QString &text) {
        return QStringLiteral("<br><span style='color:#d35400'>&#9888; %1</span>").arg(text.toHtmlEscaped());
    };
    QString html;
    if (lan) {
        const QString ip = lanAddress();
        const QString addr = ip.isEmpty() ? tr("no network connection found")
                                          : QStringLiteral("%1://%2:%3/").arg(https ? "https" : "http", ip).arg(ui->portSpin->value());
        html = tr("Other computers can connect at %1").arg(QStringLiteral("<b>%1</b>").arg(addr.toHtmlEscaped()));
        if (!hasKey)
            html += warning(tr("No API key: anyone on the network can use the server. Set a key, especially on public Wi-Fi."));
    }
    if (https) {
        if (!html.isEmpty())
            html += QStringLiteral("<br>");
        if (usesManagedCert()) {
            const QString fp = TlsCert::fingerprint(TlsCert::firstCertificate(TlsCert::caCertPath()));
            if (fp.isEmpty()) {
                html += tr("A certificate will be created when the server starts.").toHtmlEscaped();
            } else {
                const QString link = QStringLiteral("<a href='%1'>ca.pem</a>")
                                         .arg(QUrl::fromLocalFile(TlsCert::directory()).toString(QUrl::FullyEncoded));
                html += tr("To avoid browser warnings on other devices, import the CA certificate %1 there once "
                           "and trust it for websites.").arg(link);
                html += QStringLiteral("<br><small>SHA-256: %1</small>").arg(fp);
                const QString reason = TlsCert::renewalReason(TlsCert::currentNames(lanAddresses()));
                if (!reason.isEmpty())
                    html += QStringLiteral("<br>") + tr("The server certificate will be renewed at start (%1).").arg(reason).toHtmlEscaped();
            }
        } else if (ui->certFileEdit->text().trimmed().isEmpty() || ui->keyFileEdit->text().trimmed().isEmpty()) {
            html += warning(tr("Choose both the certificate and its private key, or clear both to let the launcher create them."))
                        .mid(4); // no leading line break
        }
        if (!QSslSocket::supportsSsl())
            html += warning(tr("Qt has no TLS support here, so the launcher cannot check the server's status."));
    }
    ui->networkHintLabel->setText(html);
    ui->networkHintLabel->setVisible(!html.isEmpty());
}

// ---------------------------------------------------------------------------
// HTTPS
// ---------------------------------------------------------------------------

bool MainWindow::httpsEnabled() const
{
    return ui->httpsCheck->isChecked() || ui->lanAccessCheck->isChecked();
}

bool MainWindow::usesManagedCert() const
{
    return ui->certFileEdit->text().trimmed().isEmpty() && ui->keyFileEdit->text().trimmed().isEmpty();
}

QString MainWindow::tlsCertFile() const
{
    return usesManagedCert() ? TlsCert::serverCertPath() : ui->certFileEdit->text().trimmed();
}

QString MainWindow::tlsKeyFile() const
{
    return usesManagedCert() ? TlsCert::serverKeyPath() : ui->keyFileEdit->text().trimmed();
}

bool MainWindow::prepareTls(QString *note, QString *error)
{
    if (usesManagedCert()) {
        const TlsCert::Names names = TlsCert::currentNames(lanAddresses());
        const QString reason = TlsCert::renewalReason(names);
        if (!reason.isEmpty()) {
            if (!TlsCert::generate(names, error))
                return false;
            *note = tr("Created a new server certificate (%1) for %2 in %3")
                        .arg(reason, (names.dns + names.ips).join(QStringLiteral(", ")), TlsCert::directory());
        }
        return true;
    }

    const QString cert = tlsCertFile(), key = tlsKeyFile();
    if (cert.isEmpty() || key.isEmpty()) {
        *error = tr("choose both the certificate and its private key, or clear both to let the launcher create them.");
        return false;
    }
    if (TlsCert::firstCertificate(cert).isNull()) {
        *error = tr("%1 is not a readable PEM certificate.").arg(cert);
        return false;
    }
    if (!QFileInfo(key).isReadable()) {
        *error = tr("cannot read the private key %1.").arg(key);
        return false;
    }
    return true;
}

void MainWindow::acceptPinnedCert(QNetworkReply *reply) const
{
    // The launcher talks to its own server on 127.0.0.1, which a self-signed or locally
    // issued certificate does not satisfy; trust exactly the certificate it was started with.
    const QSslCertificate pinned = m_runningCert;
    connect(reply, &QNetworkReply::sslErrors, reply, [reply, pinned](const QList<QSslError> &errors) {
        if (!pinned.isNull() && reply->sslConfiguration().peerCertificate() == pinned)
            reply->ignoreSslErrors(errors);
    });
}

void MainWindow::on_generateCertButton_clicked()
{
    const TlsCert::Names names = TlsCert::currentNames(lanAddresses());
    QString error;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = TlsCert::generate(names, &error);
    QApplication::restoreOverrideCursor();
    if (!ok) {
        QMessageBox::warning(this, tr("Warning"), tr("Could not create the certificate: %1").arg(error));
        return;
    }
    statusBar()->showMessage(tr("Created a certificate for %1").arg((names.dns + names.ips).join(QStringLiteral(", "))), 8000);
    updateNetworkHint();
}

void MainWindow::on_certBrowseButton_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select certificate"), QString(),
                                                          tr("PEM files (*.pem *.crt *.cer);;All files (*)"));
    if (!fileName.isEmpty())
        ui->certFileEdit->setText(fileName);
}

void MainWindow::on_keyBrowseButton_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select private key"),
                                                          QFileInfo(ui->certFileEdit->text()).absolutePath(),
                                                          tr("PEM files (*.pem *.key);;All files (*)"));
    if (!fileName.isEmpty())
        ui->keyFileEdit->setText(fileName);
}

// ---------------------------------------------------------------------------
// Server process
// ---------------------------------------------------------------------------

void MainWindow::toggleServer()
{
    if (m_server->isRunning()) {
        m_restartPending = false; // an explicit Stop wins over a restart requested from a hint
        stopServer();
    } else {
        startServer();
    }
}

QStringList MainWindow::buildArguments() const
{
    QStringList arguments;
    arguments << "--model" << ui->modelCombo->currentData().toString();

    if (ui->gpuAutoCheck->isChecked())
        arguments << "-ngl" << "auto";
    else
        arguments << "-ngl" << QString::number(ui->gpuLayersSpin->value());

    if (ui->contextAutoCheck->isChecked())
        arguments << "-c" << "0";
    else
        arguments << "-c" << ui->contextSizeCombo->currentText();

    // --fit adjusts the parameters left on "auto", but llama-server skips it entirely when
    // -ngl is set by hand ("n_gpu_layers already set by user ... abort"). So it is useful
    // only with automatic GPU layers; otherwise turn it off rather than have it silently fail.
    arguments << "--fit" << (ui->gpuAutoCheck->isChecked() ? "on" : "off");

    // The device list is filled by an asynchronous --list-devices probe; a server started
    // before it arrives (autostart) still has to use the saved device.
    const QString device = ui->deviceCombo->count() > 0 ? ui->deviceCombo->currentData().toString() : m_savedDevice;
    if (!device.isEmpty())
        arguments << "--device" << device;

    if (ui->moeModeCombo->isEnabled()) {
        switch (ui->moeModeCombo->currentIndex()) {
        case MoeAll: arguments << "--cpu-moe"; break;
        case MoeFirstN: arguments << "--n-cpu-moe" << QString::number(ui->moeLayersSpin->value()); break;
        default: break;
        }
    }

    switch (ui->flashAttnCombo->currentIndex()) {
    case FaOn: arguments << "-fa" << "on"; break;
    case FaOff: arguments << "-fa" << "off"; break;
    default: break; // Auto: server default
    }

    if (ui->ctxShiftCheck->isChecked())
        arguments << "--context-shift";

    // Advanced tab
    if (const int kv = ui->kvCacheCombo->currentIndex(); kv > 0)
        arguments << "-ctk" << kKvCacheTypes.at(kv) << "-ctv" << kKvCacheTypes.at(kv);
    if (ui->kvOnCpuCheck->isChecked())
        arguments << "--no-kv-offload";
    if (ui->mlockCheck->isChecked())
        arguments << "--mlock";
    if (ui->noMmapCheck->isChecked())
        arguments << "--no-mmap";
    if (!ui->threadsAutoCheck->isChecked())
        arguments << "-t" << QString::number(ui->threadsSpin->value());
    if (!ui->batchAutoCheck->isChecked())
        arguments << "-b" << QString::number(ui->batchSpin->value())
                  << "-ub" << QString::number(ui->ubatchSpin->value());
    if (!ui->parallelAutoCheck->isChecked())
        arguments << "-np" << QString::number(ui->parallelSpin->value());

    switch (ui->reasoningCombo->currentIndex()) {
    case ReasoningOn: arguments << "--reasoning" << "on"; break;
    case ReasoningOff: arguments << "--reasoning" << "off"; break;
    default: break;
    }
    if (ui->reasoningCombo->currentIndex() != ReasoningOff && !ui->reasoningUnlimitedCheck->isChecked())
        arguments << "--reasoning-budget" << QString::number(ui->reasoningBudgetSpin->value());
    if (!ui->jinjaCheck->isChecked())
        arguments << "--no-jinja";
    if (ui->mmprojCheck->isChecked() && !m_mmprojPath.isEmpty()) {
        arguments << "--mmproj" << m_mmprojPath;
        if (ui->mmprojCpuCheck->isChecked())
            arguments << "--no-mmproj-offload";
    }

    // QString::number() is locale-independent, as llama-server expects.
    if (ui->tempCheck->isChecked())
        arguments << "--temp" << QString::number(ui->tempSpin->value());
    if (ui->topPCheck->isChecked())
        arguments << "--top-p" << QString::number(ui->topPSpin->value());
    if (ui->topKCheck->isChecked())
        arguments << "--top-k" << QString::number(ui->topKSpin->value());
    if (ui->minPCheck->isChecked())
        arguments << "--min-p" << QString::number(ui->minPSpin->value());
    if (ui->presencePenaltyCheck->isChecked())
        arguments << "--presence-penalty" << QString::number(ui->presencePenaltySpin->value());
    if (ui->frequencyPenaltyCheck->isChecked())
        arguments << "--frequency-penalty" << QString::number(ui->frequencyPenaltySpin->value());

    if (ui->metricsCheck->isChecked())
        arguments << "--metrics";
    if (ui->noWarmupCheck->isChecked())
        arguments << "--no-warmup";
    if (ui->noUiCheck->isChecked())
        arguments << "--no-ui";
    if (ui->lanAccessCheck->isChecked())
        // "::" accepts IPv6 and, on Linux's default dual-stack sockets, IPv4 too, so names such as
        // host.local that resolve to an IPv6 address work. Without IPv6 in the kernel, bind IPv4 only.
        arguments << "--host" << (QFile::exists(QStringLiteral("/proc/net/if_inet6")) ? "::" : "0.0.0.0");
    if (httpsEnabled())
        arguments << "--ssl-cert-file" << tlsCertFile() << "--ssl-key-file" << tlsKeyFile();
    // The API key is passed via LLAMA_API_KEY (see startServer) so it does not show up in `ps` or the log.

    arguments << "--port" << QString::number(ui->portSpin->value());

    // Extra user-supplied arguments (appended last so they can override defaults)
    const QString extraArgs = ui->extraArgsEdit->text().trimmed();
    if (!extraArgs.isEmpty())
        arguments << QProcess::splitCommand(extraArgs);

    return arguments;
}

void MainWindow::startServer()
{
    if (m_server->isRunning())
        return;

    const QString modelPath = ui->modelCombo->currentData().toString();
    if (modelPath.isEmpty()) {
        if (isVisible())
            QMessageBox::warning(this, tr("Warning"), tr("Please select a model first."));
        else
            notify(tr("Llama Launcher"), tr("Please select a model first."), QSystemTrayIcon::Warning);
        return;
    }
    if (!QFileInfo::exists(modelPath)) {
        const QString msg = tr("Model file not found:\n%1").arg(modelPath);
        if (isVisible())
            QMessageBox::warning(this, tr("Warning"), msg);
        else
            notify(tr("Llama Launcher"), msg, QSystemTrayIcon::Warning);
        return;
    }

    QString tlsNote;
    if (httpsEnabled()) {
        QString error;
        if (!prepareTls(&tlsNote, &error)) {
            const QString msg = tr("HTTPS: %1").arg(error);
            if (isVisible())
                QMessageBox::warning(this, tr("Warning"), msg);
            else
                notify(tr("Llama Launcher"), msg, QSystemTrayIcon::Warning);
            return;
        }
    }

    const QString exe = serverExecutable();
    const QFileInfo exeInfo(exe.contains(QLatin1Char('/')) ? exe : QStandardPaths::findExecutable(exe));
    if (!exeInfo.isFile() || !exeInfo.isExecutable()) {
        const QString msg = tr("Could not start %1: %2").arg(exe, processErrorText(QProcess::FailedToStart));
        ui->consoleOutput->appendPlainText(msg);
        ui->tabWidget->setCurrentWidget(ui->logTab);
        notify(tr("Llama server failed to start"), msg, QSystemTrayIcon::Critical);
        return;
    }

    saveSettings(); // persist what is about to be used, even if the session ends abruptly
    addRecentModel(modelPath);

    ServerProcess::Info info;
    info.exe = exeInfo.absoluteFilePath();
    info.args = buildArguments();
    info.apiKey = ui->apiKeyEdit->text().trimmed();
    info.modelName = QFileInfo(modelPath).fileName();
    info.port = ui->portSpin->value();
    info.webUi = !ui->noUiCheck->isChecked();
    info.lan = ui->lanAccessCheck->isChecked();
    info.https = httpsEnabled();
    if (info.https)
        info.certPem = TlsCert::firstCertificate(tlsCertFile()).toPem();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("LLAMA_API_KEY"));
    if (!info.apiKey.isEmpty())
        env.insert(QStringLiteral("LLAMA_API_KEY"), info.apiKey);

    ui->consoleOutput->clear();
    m_logAdvisor.reset();
    m_appliedFixes.clear();
    m_restartPending = false;
    updateLogAdvice();
    if (!tlsNote.isEmpty())
        ui->consoleOutput->appendPlainText(tlsNote);
    ui->consoleOutput->appendPlainText(tr("Starting %1 %2").arg(info.exe, joinCommand(info.args)));

    QString error;
    if (!m_server->start(info, env, &error)) {
        const QString msg = tr("Could not start %1: %2").arg(info.exe, error);
        ui->consoleOutput->appendPlainText(msg);
        ui->tabWidget->setCurrentWidget(ui->logTab);
        notify(tr("Llama server failed to start"), msg, QSystemTrayIcon::Critical);
        return;
    }
    serverStarted();
}

bool MainWindow::adoptRunningServer()
{
    QStringList output;
    if (!m_server->adopt(&output))
        return false;

    ui->consoleOutput->clear();
    m_logAdvisor.reset();
    m_appliedFixes.clear();
    for (const QString &line : std::as_const(output)) {
        ui->consoleOutput->appendPlainText(line);
        m_lastOutputLine = line;
        m_logAdvisor.feedLine(line);
    }
    const ServerProcess::Info &info = m_server->info();
    ui->consoleOutput->appendPlainText(
        tr("Picked up the server that was already running (started %1).")
            .arg(QLocale().toString(info.startedAt, QLocale::ShortFormat)));
    updateLogAdvice();
    serverStarted();
    if (m_server->serverPid() > 0)
        m_monitor->setProcessId(m_server->serverPid());
    return true;
}

void MainWindow::serverStarted()
{
    const ServerProcess::Info &info = m_server->info();
    m_stopRequested = false;
    m_runningModelName = info.modelName;
    m_runningPort = info.port;
    m_runningWebUi = info.webUi;
    m_runningLan = info.lan;
    m_runningHttps = info.https;
    m_runningCert = info.https ? QSslCertificate(info.certPem) : QSslCertificate();
    m_monitor->resetServerStats();
    setStatus(Status::Loading);
    m_healthTimer->start();
}

void MainWindow::stopServer()
{
    if (!m_server->isRunning())
        return;
    ui->consoleOutput->appendPlainText(tr("Stopping server..."));
    m_stopRequested = true;
    m_server->terminate();
    m_killTimer->start();
}

void MainWindow::handleServerOutput(const QStringList &lines)
{
    for (const QString &trimmed : lines) {
        ui->consoleOutput->appendPlainText(trimmed);
        m_lastOutputLine = trimmed;
        m_monitor->feedLogLine(trimmed);
        if (m_logAdvisor.feedLine(trimmed)) {
            updateLogAdvice();
            const int count = int(m_logAdvisor.hints().size());
            statusBar()->showMessage(tr("The server log has %n hint(s), see the Log tab", nullptr, count), 8000);
        }
    }
}

void MainWindow::updateLogAdvice()
{
    const QList<LogAdvisor::Hint> &hints = m_logAdvisor.hints();
    ui->logAdviceLabel->setVisible(!hints.isEmpty());
    if (hints.isEmpty()) {
        ui->logAdviceLabel->clear();
        return;
    }

    QString html = QStringLiteral("<b>%1</b><ul style='margin:0'>").arg(tr("Hints from the server log:").toHtmlEscaped());
    for (const LogAdvisor::Hint &hint : hints) {
        html += QStringLiteral("<li>") + hint.text.toHtmlEscaped();
        const QString fix = hintFixLabel(hint.id);
        if (m_appliedFixes.contains(hint.id))
            html += QStringLiteral(" <i>(%1)</i>").arg(tr("changed, takes effect after a restart").toHtmlEscaped());
        else if (!fix.isEmpty())
            html += QStringLiteral(" <a href='fix:%1'>%2</a>").arg(int(hint.id)).arg(fix.toHtmlEscaped());
        html += QStringLiteral("</li>");
    }
    html += QStringLiteral("</ul>");
    if (!m_appliedFixes.isEmpty() && !m_restartPending) {
        const bool running = m_server->isRunning();
        html += QStringLiteral("<a href='restart'>%1</a>")
                    .arg((running ? tr("Restart the server") : tr("Start the server")).toHtmlEscaped());
    }
    ui->logAdviceLabel->setText(QStringLiteral("<span style='color:#d35400'>&#9888;</span> ") + html);
}

QString MainWindow::hintFixLabel(LogAdvisor::HintId id)
{
    switch (id) {
    case LogAdvisor::HintId::FitSkipped: return tr("Set GPU layers to Auto");
    case LogAdvisor::HintId::ContextShift: return tr("Turn off Context Shift");
    // Flash Attention: the server already runs without it, and setting it to Off by hand
    // would only add the "quantized KV cache needs Flash Attention" warning.
    case LogAdvisor::HintId::FlashAttnOff:
    case LogAdvisor::HintId::OutOfMemory: // several possible remedies, the user has to choose
        break;
    }
    return {};
}

void MainWindow::applyHintFix(LogAdvisor::HintId id)
{
    switch (id) {
    case LogAdvisor::HintId::FitSkipped: ui->gpuAutoCheck->setChecked(true); break;
    case LogAdvisor::HintId::ContextShift: ui->ctxShiftCheck->setChecked(false); break;
    case LogAdvisor::HintId::FlashAttnOff:
    case LogAdvisor::HintId::OutOfMemory:
        return;
    }
    saveSettings();
    if (!m_appliedFixes.contains(id))
        m_appliedFixes << id;
    updateLogAdvice();
}

void MainWindow::restartServer()
{
    if (!m_server->isRunning()) {
        startServer();
    } else {
        m_restartPending = true; // handleProcessFinished() starts it again
        updateLogAdvice();
        stopServer();
    }
}

void MainWindow::onLogAdviceLinkActivated(const QString &link)
{
    if (link == QLatin1String("restart")) {
        restartServer();
    } else if (link.startsWith(QLatin1String("fix:"))) {
        applyHintFix(LogAdvisor::HintId(link.mid(4).toInt()));
    }
}

void MainWindow::on_copyLogButton_clicked()
{
    QApplication::clipboard()->setText(ui->consoleOutput->toPlainText());
    statusBar()->showMessage(tr("Log copied to clipboard"), 3000);
}

void MainWindow::on_clearLogButton_clicked()
{
    ui->consoleOutput->clear();
}

void MainWindow::handleServerFinished(int exitCode, int crashSignal)
{
    m_killTimer->stop();
    m_healthTimer->stop();

    QString result;
    if (crashSignal != 0)
        result = m_stopRequested ? tr("Server stopped (signal %1).").arg(crashSignal)
                                 : tr("Server crashed (signal %1).").arg(crashSignal);
    else if (exitCode >= 0)
        result = tr("Server stopped (exit code %1).").arg(exitCode);
    else
        result = tr("Server stopped.");
    ui->consoleOutput->appendPlainText(result);

    const bool unexpected = !m_stopRequested && (crashSignal != 0 || exitCode != 0);
    setStatus(Status::Stopped);

    if (unexpected) {
        ui->tabWidget->setCurrentWidget(ui->logTab); // the reason is in the server output
        QString msg = exitCode > 0 ? tr("Server stopped unexpectedly (exit code %1).").arg(exitCode) : result;
        if (!m_lastOutputLine.isEmpty())
            msg += QLatin1Char('\n') + m_lastOutputLine.right(200);
        notify(tr("Llama server stopped"), msg, QSystemTrayIcon::Warning);
    }

    if (m_restartPending) {
        m_restartPending = false;
        QTimer::singleShot(0, this, &MainWindow::startServer);
    } else {
        updateLogAdvice(); // "Restart the server" becomes "Start the server"
    }
}

void MainWindow::checkServerHealth()
{
    if (!m_server->isRunning()) {
        m_healthTimer->stop();
        return;
    }

    QNetworkRequest request(serverUrl().resolved(QUrl(QStringLiteral("health"))));
    request.setTransferTimeout(1500); // shorter than the 2 s poll interval, so replies don't pile up
    QNetworkReply *reply = m_networkManager->get(request);
    acceptPinnedCert(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (!m_server->isRunning())
            return;
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError && statusCode == 200)
            setStatus(Status::Running);
        else if (m_status != Status::Loading)
            setStatus(Status::Loading); // 503 while loading, or not listening yet
    });
}

// ---------------------------------------------------------------------------
// llama-server binary & devices
// ---------------------------------------------------------------------------

void MainWindow::on_serverBrowseButton_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select llama-server"),
                                                          QFileInfo(serverExecutable()).absolutePath());
    if (!fileName.isEmpty()) {
        ui->serverPathEdit->setText(fileName);
        refreshServerInfo();
    }
}

void MainWindow::on_refreshDevicesButton_clicked()
{
    refreshServerInfo();
}

QString MainWindow::serverExecutable() const
{
    const QString path = ui->serverPathEdit->text().trimmed();
    if (!path.isEmpty())
        return QDir::cleanPath(path.startsWith(QLatin1String("~/")) ? QDir::homePath() + path.mid(1) : path);
    const QString found = QStandardPaths::findExecutable(QStringLiteral("llama-server"));
    return found.isEmpty() ? QStringLiteral("llama-server") : found;
}

void MainWindow::runServerProbe(const QStringList &args, std::function<void(bool, const QString &)> onDone)
{
    auto *probe = new QProcess(this);
    probe->setProcessChannelMode(QProcess::MergedChannels);
    const int generation = m_probeGeneration;

    auto finish = [this, probe, generation, onDone](bool ok) {
        // Ignore results from probes started before the server path changed again.
        if (generation == m_probeGeneration)
            onDone(ok, QString::fromUtf8(probe->readAll()));
        probe->deleteLater();
    };
    connect(probe, &QProcess::finished, this, [finish](int code, QProcess::ExitStatus status) {
        finish(status == QProcess::NormalExit && code == 0);
    });
    connect(probe, &QProcess::errorOccurred, this, [finish](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart)
            finish(false);
    });
    probe->start(serverExecutable(), args);
}

void MainWindow::refreshServerInfo()
{
    ++m_probeGeneration;

    // Remember the current choice so it survives the repopulation.
    if (ui->deviceCombo->count() > 0)
        m_savedDevice = ui->deviceCombo->currentData().toString();

    const QString exe = serverExecutable();
    ui->serverVersionLabel->setText(tr("Checking %1...").arg(exe.toHtmlEscaped()));
    ui->refreshDevicesButton->setEnabled(false);

    runServerProbe({QStringLiteral("--version")}, [this, exe](bool ok, const QString &out) {
        static const QRegularExpression re(QStringLiteral("^version:\\s*(.+)$"), QRegularExpression::MultilineOption);
        const auto m = re.match(out);
        if (ok && m.hasMatch())
            ui->serverVersionLabel->setText(tr("%1 &mdash; version %2")
                                                .arg(exe.toHtmlEscaped(), m.captured(1).trimmed().toHtmlEscaped()));
        else
            ui->serverVersionLabel->setText(QStringLiteral("<span style='color:#c0392b'>%1</span>")
                                                .arg(tr("Cannot run %1").arg(exe).toHtmlEscaped()));
    });

    runServerProbe({QStringLiteral("--list-devices")}, [this](bool ok, const QString &out) {
        populateDevices(ok ? out : QString());
        updateUiState();
    });
}

void MainWindow::populateDevices(const QString &listOutput)
{
    const QSignalBlocker blocker(ui->deviceCombo);
    ui->deviceCombo->clear();
    ui->deviceCombo->addItem(tr("All GPUs (auto)"), QString());

    // Output format:  "  Vulkan1: AMD Radeon Pro WX Series (RADV POLARIS11) (4096 MiB, 4090 MiB free)"
    static const QRegularExpression re(QStringLiteral("^\\s+([A-Za-z]+\\d+):\\s+(.+)$"), QRegularExpression::MultilineOption);
    static const QRegularExpression freeRe(QStringLiteral("(\\d+) MiB free"));
    auto it = re.globalMatch(listOutput.section(QStringLiteral("Available devices:"), 1));
    while (it.hasNext()) {
        const auto m = it.next();
        const QString desc = m.captured(2).trimmed();
        if (m_gpuName.isEmpty() && desc.contains(QLatin1String("AMD")))
            m_gpuName = desc.section(QStringLiteral(" ("), 0, 0); // "AMD Radeon Pro WX Series"
        ui->deviceCombo->addItem(QStringLiteral("%1: %2").arg(m.captured(1), desc), m.captured(1));
        const auto fm = freeRe.match(desc);
        if (fm.hasMatch())
            ui->deviceCombo->setItemData(ui->deviceCombo->count() - 1, fm.captured(1).toLongLong(), kDeviceFreeMiBRole);
    }
    ui->deviceCombo->addItem(tr("CPU only"), QStringLiteral("none"));

    selectDevice(m_savedDevice);
    updateFitWarning();
}

void MainWindow::selectDevice(const QString &id)
{
    m_savedDevice = id;
    if (ui->deviceCombo->count() == 0)
        return; // applied once the device list arrives
    int idx = ui->deviceCombo->findData(id);
    if (idx < 0) {
        // Keep a saved device that is not detected right now instead of silently dropping it.
        ui->deviceCombo->addItem(tr("%1 (not detected)").arg(id), id);
        idx = ui->deviceCombo->count() - 1;
    }
    ui->deviceCombo->setCurrentIndex(idx);
}

// ---------------------------------------------------------------------------
// Models
// ---------------------------------------------------------------------------

void MainWindow::on_browseButton_clicked()
{
    QString startDir = QFileInfo(ui->modelCombo->currentData().toString()).absolutePath();
    if (ui->modelCombo->currentData().toString().isEmpty())
        startDir = ui->modelsDirEdit->text();
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select Model"), startDir,
                                                          tr("GGUF Models (*.gguf);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    addRecentModel(fileName);
    rescanModels(fileName);
}

void MainWindow::on_modelsDirButton_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Models Folder"), ui->modelsDirEdit->text());
    if (dir.isEmpty())
        return;
    ui->modelsDirEdit->setText(dir);
    rescanModels();
}

void MainWindow::addRecentModel(const QString &path)
{
    QSettings settings;
    QStringList recent = settings.value("recentModels").toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecentModels)
        recent.removeLast();
    settings.setValue("recentModels", recent);
}

void MainWindow::rescanModels(const QString &selectPath)
{
    const QString wanted = selectPath.isEmpty() ? ui->modelCombo->currentData().toString() : selectPath;

    QStringList paths;
    const QString dir = ui->modelsDirEdit->text().trimmed();
    if (!dir.isEmpty() && QFileInfo(dir).isDir()) {
        QDirIterator it(dir, {QStringLiteral("*.gguf")}, QDir::Files,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            const QString path = it.next();
            if (!isAuxiliaryGgufFile(it.fileName()))
                paths << path;
        }
    }
    paths.sort(Qt::CaseInsensitive);

    // Recently used models living outside the models folder.
    const QStringList recent = QSettings().value("recentModels").toStringList();
    for (const QString &path : recent) {
        if (!paths.contains(path) && QFileInfo::exists(path))
            paths << path;
    }
    if (!wanted.isEmpty() && !paths.contains(wanted))
        paths << wanted; // keep the selection even if the file vanished; startServer() reports it

    {
        const QSignalBlocker blocker(ui->modelCombo);
        ui->modelCombo->clear();
        const QDir base(dir);
        for (const QString &path : paths) {
            const QFileInfo fi(path);
            QString label = (!dir.isEmpty() && path.startsWith(base.absolutePath() + QLatin1Char('/')))
                                ? base.relativeFilePath(path)
                                : path;
            if (path.startsWith(QDir::homePath() + QLatin1Char('/')) && label == path)
                label = QStringLiteral("~") + path.mid(QDir::homePath().size());
            if (fi.exists())
                label += QStringLiteral("  (%1)").arg(formatBytes(fi.size()));
            else
                label += QStringLiteral("  ") + tr("(missing)");
            ui->modelCombo->addItem(label, path);
            ui->modelCombo->setItemData(ui->modelCombo->count() - 1, path, Qt::ToolTipRole);
        }
        ui->modelCombo->setCurrentIndex(ui->modelCombo->findData(wanted));
    }
    onModelChanged(ui->modelCombo->currentIndex());
}

void MainWindow::onModelChanged(int index)
{
    const QString path = index >= 0 ? ui->modelCombo->itemData(index).toString() : QString();
    if (path == m_currentModelPath && !path.isEmpty()) {
        updateModelInfo();
        return;
    }
    // Remember the settings of the model we are leaving, then show the new model's.
    if (!m_currentModelPath.isEmpty() && !m_loading)
        saveModelSettings(m_currentModelPath);
    m_currentModelPath = path;
    if (!path.isEmpty())
        loadModelSettings(path);
    updateModelInfo();
}

void MainWindow::updateModelInfo()
{
    const QString path = m_currentModelPath;
    m_mmprojPath = path.isEmpty() ? QString() : findMmproj(path);
    ui->mmprojFileLabel->setText(m_mmprojPath.isEmpty()
                                     ? tr("No mmproj*.gguf file next to the model.")
                                     : QFileInfo(m_mmprojPath).fileName());
    if (path.isEmpty()) {
        m_modelInfo = GgufInfo();
        ui->modelInfoLabel->setText(tr("No model selected."));
        updateUiState();
        updateFitWarning();
        return;
    }

    m_modelInfo = readGgufInfo(path);
    if (!m_modelInfo.valid) {
        ui->modelInfoLabel->setText(QStringLiteral("<span style='color:#c0392b'>%1</span>")
                                        .arg(tr("Cannot read model metadata: %1").arg(m_modelInfo.error).toHtmlEscaped()));
    } else {
        QStringList parts;
        QString title = m_modelInfo.name.isEmpty() ? m_modelInfo.architecture : m_modelInfo.name;
        if (!m_modelInfo.sizeLabel.isEmpty() && !title.contains(m_modelInfo.sizeLabel))
            title += QLatin1Char(' ') + m_modelInfo.sizeLabel;
        parts << QStringLiteral("<b>%1</b>").arg(title.toHtmlEscaped());
        parts << formatBytes(m_modelInfo.totalSize);
        if (m_modelInfo.splitCount > 1)
            parts << tr("%n parts", nullptr, m_modelInfo.splitCount);
        if (m_modelInfo.blockCount)
            parts << tr("%n layers", nullptr, int(m_modelInfo.blockCount));
        if (m_modelInfo.contextLength)
            parts << tr("native context %1").arg(m_modelInfo.contextLength);
        if (m_modelInfo.isMoe())
            parts << tr("MoE: %1 experts, %2 active").arg(m_modelInfo.expertCount).arg(m_modelInfo.expertUsedCount);
        ui->modelInfoLabel->setText(parts.join(QStringLiteral(" &middot; ")));
    }
    updateUiState();
    updateFitWarning();
}

void MainWindow::updateFitWarning()
{
    ui->fitWarningLabel->clear();
    ui->fitWarningLabel->setVisible(false);
    if (!m_modelInfo.valid || m_modelInfo.totalSize <= 0 || ui->deviceCombo->count() == 0)
        return;

    const QString device = ui->deviceCombo->currentData().toString();
    if (device == QLatin1String("none"))
        return; // CPU only: nothing to fit

    // Free VRAM of the chosen device, or of all GPUs together for "auto".
    qint64 freeMiB = 0;
    QString where;
    if (device.isEmpty()) {
        for (int i = 0; i < ui->deviceCombo->count(); ++i)
            freeMiB += ui->deviceCombo->itemData(i, kDeviceFreeMiBRole).toLongLong();
        where = tr("all GPUs");
    } else {
        freeMiB = ui->deviceCombo->currentData(kDeviceFreeMiBRole).toLongLong();
        where = device;
    }
    if (freeMiB <= 0)
        return;

    const qint64 freeBytes = freeMiB * 1024 * 1024;
    // Weights alone; the KV cache and compute buffers need roughly another 10-20 %.
    const bool fits = double(m_modelInfo.totalSize) < double(freeBytes) * 0.85;
    QString html;
    if (fits) {
        html = QStringLiteral("<span style='color:#27ae60'>&#10003; %1</span>")
                   .arg(tr("Model (%1) fits into free VRAM on %2 (%3).")
                            .arg(formatBytes(m_modelInfo.totalSize), where, formatBytes(freeBytes))
                            .toHtmlEscaped());
    } else {
        QString msg = tr("Model (%1) is larger than free VRAM on %2 (%3): part of it will run on the CPU, which is slower.")
                          .arg(formatBytes(m_modelInfo.totalSize), where, formatBytes(freeBytes));
        if (m_modelInfo.isMoe() && ui->moeModeCombo->currentIndex() == MoeAuto)
            msg += QLatin1Char(' ') + tr("For this MoE model, keeping the experts on the CPU (option below) usually gives the best speed.");
        html = QStringLiteral("<span style='color:#d35400'>&#9888; %1</span>").arg(msg.toHtmlEscaped());
    }
    ui->fitWarningLabel->setText(html);
    ui->fitWarningLabel->setVisible(true);
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void MainWindow::updateUiState()
{
    ui->gpuLayersSpin->setDisabled(ui->gpuAutoCheck->isChecked());
    ui->contextSizeCombo->setDisabled(ui->contextAutoCheck->isChecked());

    ui->threadsSpin->setDisabled(ui->threadsAutoCheck->isChecked());
    ui->batchSpin->setDisabled(ui->batchAutoCheck->isChecked());
    ui->ubatchSpin->setDisabled(ui->batchAutoCheck->isChecked());
    ui->parallelSpin->setDisabled(ui->parallelAutoCheck->isChecked());
    const bool thinking = ui->reasoningCombo->currentIndex() != ReasoningOff;
    ui->labelReasoningBudget->setEnabled(thinking);
    ui->reasoningUnlimitedCheck->setEnabled(thinking);
    ui->reasoningBudgetSpin->setEnabled(thinking && !ui->reasoningUnlimitedCheck->isChecked());
    ui->mmprojCheck->setEnabled(!m_mmprojPath.isEmpty());
    ui->mmprojCpuCheck->setEnabled(!m_mmprojPath.isEmpty() && ui->mmprojCheck->isChecked());
    ui->tempSpin->setEnabled(ui->tempCheck->isChecked());
    ui->topPSpin->setEnabled(ui->topPCheck->isChecked());
    ui->topKSpin->setEnabled(ui->topKCheck->isChecked());
    ui->minPSpin->setEnabled(ui->minPCheck->isChecked());
    ui->presencePenaltySpin->setEnabled(ui->presencePenaltyCheck->isChecked());
    ui->frequencyPenaltySpin->setEnabled(ui->frequencyPenaltyCheck->isChecked());

    const bool kvNeedsFa = ui->kvCacheCombo->currentIndex() > 0 && ui->flashAttnCombo->currentIndex() == FaOff;
    ui->kvWarningLabel->setVisible(kvNeedsFa);
    if (kvNeedsFa)
        ui->kvWarningLabel->setText(QStringLiteral("<span style='color:#d35400'>&#9888; %1</span>")
                                        .arg(tr("A quantized KV cache does not work with Flash Attention forced off. "
                                                "Set Flash Attention to Auto or On on the Model tab; "
                                                "the server decides by itself whether your GPU can use it.").toHtmlEscaped()));

    // MoE options only make sense for MoE models; keep them enabled if metadata is unknown.
    const bool moeApplicable = !m_modelInfo.valid || m_modelInfo.isMoe();
    ui->moeModeCombo->setEnabled(moeApplicable);
    ui->moeLayersSpin->setEnabled(moeApplicable && ui->moeModeCombo->currentIndex() == MoeFirstN);
    ui->labelMoe->setEnabled(moeApplicable);
    if (m_modelInfo.blockCount)
        ui->moeLayersSpin->setMaximum(int(m_modelInfo.blockCount));

    // Everything stays editable while the server runs: changes apply at the next start, and
    // updatePendingChanges() offers "Restart to Apply" with the list of differences.
    const bool lan = ui->lanAccessCheck->isChecked();
    ui->httpsCheck->setEnabled(!lan);
    ui->httpsCheck->setToolTip(lan ? tr("Always on while access from the local network is allowed: "
                                        "nothing leaves this computer unencrypted.")
                                   : tr("Serve over HTTPS so prompts, answers and the API key cannot be read "
                                        "on the network (--ssl-cert-file / --ssl-key-file)."));
    const bool tlsEditable = httpsEnabled();
    for (QWidget *w : {static_cast<QWidget *>(ui->certFileEdit), static_cast<QWidget *>(ui->keyFileEdit),
                       static_cast<QWidget *>(ui->certBrowseButton), static_cast<QWidget *>(ui->keyBrowseButton)})
        w->setEnabled(tlsEditable);
    ui->generateCertButton->setEnabled(tlsEditable && usesManagedCert());
}

QString MainWindow::autostartFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + QStringLiteral("/autostart/llama-launcher.desktop");
}

void MainWindow::onLaunchAtLoginToggled(bool enabled)
{
    if (m_loading)
        return;
    const QString path = autostartFilePath();
    if (!enabled) {
        QFile::remove(path);
        return;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        const QString exe = QCoreApplication::applicationFilePath();
        file.write(QStringLiteral("[Desktop Entry]\n"
                                  "Type=Application\n"
                                  "Name=Llama Launcher\n"
                                  "Exec=\"%1\" --autostart\n"
                                  "Icon=llama-launcher\n"
                                  "Terminal=false\n"
                                  "X-GNOME-Autostart-enabled=true\n")
                       .arg(exe)
                       .toUtf8());
    }
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Warning"), tr("Could not write %1").arg(path));
        const QSignalBlocker blocker(ui->launchAtLoginCheck);
        ui->launchAtLoginCheck->setChecked(false);
    }
}

void MainWindow::migrateLegacySettings()
{
    // Settings used to live under the placeholder organization "MyCompany".
    QSettings settings;
    if (settings.contains("migratedFromLegacy"))
        return;
    QSettings legacy("MyCompany", "LlamaLauncher");
    const QStringList keys = legacy.allKeys();
    for (const QString &key : keys) {
        if (!settings.contains(key))
            settings.setValue(key, legacy.value(key));
    }
    settings.setValue("migratedFromLegacy", true);
}

void MainWindow::saveModelSettings(const QString &modelPath)
{
    QSettings settings;
    settings.beginGroup(modelSettingsGroup(modelPath));
    settings.setValue("path", modelPath);
    settings.setValue("contextSize", ui->contextSizeCombo->currentText());
    settings.setValue("contextAuto", ui->contextAutoCheck->isChecked());
    settings.setValue("gpuLayers", ui->gpuLayersSpin->value());
    settings.setValue("gpuAuto", ui->gpuAutoCheck->isChecked());
    settings.setValue("device", ui->deviceCombo->count() > 0 ? ui->deviceCombo->currentData().toString() : m_savedDevice);
    settings.setValue("moeMode", ui->moeModeCombo->currentIndex());
    settings.setValue("moeLayers", ui->moeLayersSpin->value());
    settings.setValue("flashAttn", ui->flashAttnCombo->currentIndex());
    settings.setValue("contextShift", ui->ctxShiftCheck->isChecked());
    settings.setValue("extraArgs", ui->extraArgsEdit->text());
    settings.setValue("kvCache", kKvCacheTypes.at(qMax(0, ui->kvCacheCombo->currentIndex())));
    settings.setValue("kvOnCpu", ui->kvOnCpuCheck->isChecked());
    settings.setValue("mlock", ui->mlockCheck->isChecked());
    settings.setValue("noMmap", ui->noMmapCheck->isChecked());
    settings.setValue("threads", ui->threadsSpin->value());
    settings.setValue("threadsAuto", ui->threadsAutoCheck->isChecked());
    settings.setValue("batch", ui->batchSpin->value());
    settings.setValue("ubatch", ui->ubatchSpin->value());
    settings.setValue("batchAuto", ui->batchAutoCheck->isChecked());
    settings.setValue("parallel", ui->parallelSpin->value());
    settings.setValue("parallelAuto", ui->parallelAutoCheck->isChecked());
    settings.setValue("reasoning", ui->reasoningCombo->currentIndex());
    settings.setValue("reasoningBudget", ui->reasoningBudgetSpin->value());
    settings.setValue("reasoningUnlimited", ui->reasoningUnlimitedCheck->isChecked());
    settings.setValue("jinja", ui->jinjaCheck->isChecked());
    settings.setValue("mmproj", ui->mmprojCheck->isChecked());
    settings.setValue("mmprojCpu", ui->mmprojCpuCheck->isChecked());
    settings.setValue("temp", ui->tempCheck->isChecked());
    settings.setValue("tempValue", ui->tempSpin->value());
    settings.setValue("topP", ui->topPCheck->isChecked());
    settings.setValue("topPValue", ui->topPSpin->value());
    settings.setValue("topK", ui->topKCheck->isChecked());
    settings.setValue("topKValue", ui->topKSpin->value());
    settings.setValue("minP", ui->minPCheck->isChecked());
    settings.setValue("minPValue", ui->minPSpin->value());
    settings.setValue("presencePenalty", ui->presencePenaltyCheck->isChecked());
    settings.setValue("presencePenaltyValue", ui->presencePenaltySpin->value());
    settings.setValue("frequencyPenalty", ui->frequencyPenaltyCheck->isChecked());
    settings.setValue("frequencyPenaltyValue", ui->frequencyPenaltySpin->value());
    settings.setValue("metrics", ui->metricsCheck->isChecked());
    settings.setValue("noWarmup", ui->noWarmupCheck->isChecked());
    settings.endGroup();
}

void MainWindow::loadModelSettings(const QString &modelPath)
{
    QSettings settings;
    const QString group = modelSettingsGroup(modelPath);
    if (!settings.contains(group + QStringLiteral("/path"))) {
        migrateExtraArgs(); // first time with this model: keep the current values as its starting point
        return;
    }
    settings.beginGroup(group);
    const QString contextSize = settings.value("contextSize").toString();
    if (!contextSize.isEmpty())
        ui->contextSizeCombo->setCurrentText(contextSize);
    ui->contextAutoCheck->setChecked(settings.value("contextAuto", false).toBool());
    ui->gpuLayersSpin->setValue(settings.value("gpuLayers", 35).toInt());
    ui->gpuAutoCheck->setChecked(settings.value("gpuAuto", false).toBool());
    selectDevice(settings.value("device").toString());
    ui->moeModeCombo->setCurrentIndex(qBound(0, settings.value("moeMode", 0).toInt(), 2));
    ui->moeLayersSpin->setValue(settings.value("moeLayers", 20).toInt());
    ui->flashAttnCombo->setCurrentIndex(qBound(0, settings.value("flashAttn", 0).toInt(), 2));
    ui->ctxShiftCheck->setChecked(settings.value("contextShift", false).toBool());
    ui->extraArgsEdit->setText(settings.value("extraArgs").toString());

    // Settings saved before these options existed lack the keys; fall back to the defaults.
    ui->kvCacheCombo->setCurrentIndex(qMax(0, kKvCacheTypes.indexOf(settings.value("kvCache").toString())));
    ui->kvOnCpuCheck->setChecked(settings.value("kvOnCpu", false).toBool());
    ui->mlockCheck->setChecked(settings.value("mlock", false).toBool());
    ui->noMmapCheck->setChecked(settings.value("noMmap", false).toBool());
    ui->threadsSpin->setValue(settings.value("threads", qMax(1, QThread::idealThreadCount() / 2)).toInt());
    ui->threadsAutoCheck->setChecked(settings.value("threadsAuto", true).toBool());
    ui->batchSpin->setValue(settings.value("batch", 2048).toInt());
    ui->ubatchSpin->setValue(settings.value("ubatch", 512).toInt());
    ui->batchAutoCheck->setChecked(settings.value("batchAuto", true).toBool());
    ui->parallelSpin->setValue(settings.value("parallel", 1).toInt());
    ui->parallelAutoCheck->setChecked(settings.value("parallelAuto", true).toBool());
    ui->reasoningCombo->setCurrentIndex(qBound(0, settings.value("reasoning", 0).toInt(), 2));
    ui->reasoningBudgetSpin->setValue(settings.value("reasoningBudget", 2048).toInt());
    ui->reasoningUnlimitedCheck->setChecked(settings.value("reasoningUnlimited", true).toBool());
    ui->jinjaCheck->setChecked(settings.value("jinja", true).toBool());
    ui->mmprojCheck->setChecked(settings.value("mmproj", false).toBool());
    ui->mmprojCpuCheck->setChecked(settings.value("mmprojCpu", false).toBool());
    ui->tempCheck->setChecked(settings.value("temp", false).toBool());
    ui->tempSpin->setValue(settings.value("tempValue", 0.8).toDouble());
    ui->topPCheck->setChecked(settings.value("topP", false).toBool());
    ui->topPSpin->setValue(settings.value("topPValue", 0.95).toDouble());
    ui->topKCheck->setChecked(settings.value("topK", false).toBool());
    ui->topKSpin->setValue(settings.value("topKValue", 40).toInt());
    ui->minPCheck->setChecked(settings.value("minP", false).toBool());
    ui->minPSpin->setValue(settings.value("minPValue", 0.05).toDouble());
    ui->presencePenaltyCheck->setChecked(settings.value("presencePenalty", false).toBool());
    ui->presencePenaltySpin->setValue(settings.value("presencePenaltyValue", 1.5).toDouble());
    ui->frequencyPenaltyCheck->setChecked(settings.value("frequencyPenalty", false).toBool());
    ui->frequencyPenaltySpin->setValue(settings.value("frequencyPenaltyValue", 0.5).toDouble());
    ui->metricsCheck->setChecked(settings.value("metrics", false).toBool());
    ui->noWarmupCheck->setChecked(settings.value("noWarmup", false).toBool());
    settings.endGroup();

    migrateExtraArgs();
}

int MainWindow::migrateExtraArgs()
{
    const QStringList tokens = QProcess::splitCommand(ui->extraArgsEdit->text());
    if (tokens.isEmpty())
        return 0;

    // Accept --flag=value as well as --flag value.
    QStringList args;
    for (const QString &t : tokens) {
        const int eq = t.indexOf(QLatin1Char('='));
        if (t.startsWith(QLatin1String("--")) && eq > 2)
            args << t.left(eq) << t.mid(eq + 1);
        else
            args << t;
    }

    QStringList rest, kvTokens;
    QString kvK, kvV;
    int batch = 0, ubatch = 0, moved = 0;

    for (int i = 0; i < args.size(); ++i) {
        const QString &a = args.at(i);
        const QString v = i + 1 < args.size() ? args.at(i + 1) : QString();
        bool isInt = false, isNum = false;
        const int iv = v.toInt(&isInt);
        const double dv = v.toDouble(&isNum);
        auto is = [&a](std::initializer_list<const char *> names) {
            for (const char *n : names)
                if (a == QLatin1String(n))
                    return true;
            return false;
        };
        auto flag = [&](QCheckBox *check, bool on) { check->setChecked(on); ++moved; };
        auto valued = [&](auto apply) { apply(); ++i; ++moved; };

        if (is({"-ctk", "--cache-type-k"}) && !v.isEmpty()) { kvK = v; kvTokens << a << v; ++i; }
        else if (is({"-ctv", "--cache-type-v"}) && !v.isEmpty()) { kvV = v; kvTokens << a << v; ++i; }
        else if (is({"-nkvo", "--no-kv-offload"})) flag(ui->kvOnCpuCheck, true);
        else if (is({"-kvo", "--kv-offload"})) flag(ui->kvOnCpuCheck, false);
        else if (is({"--mlock"})) flag(ui->mlockCheck, true);
        else if (is({"--no-mmap"})) flag(ui->noMmapCheck, true);
        else if (is({"--mmap"})) flag(ui->noMmapCheck, false);
        else if (is({"-t", "--threads"}) && isInt)
            valued([&] { ui->threadsAutoCheck->setChecked(iv <= 0); if (iv > 0) ui->threadsSpin->setValue(iv); });
        else if (is({"-b", "--batch-size"}) && isInt && iv > 0) valued([&] { batch = iv; });
        else if (is({"-ub", "--ubatch-size"}) && isInt && iv > 0) valued([&] { ubatch = iv; });
        else if (is({"-np", "--parallel"}) && isInt)
            valued([&] { ui->parallelAutoCheck->setChecked(iv <= 0); if (iv > 0) ui->parallelSpin->setValue(iv); });
        else if (is({"-rea", "--reasoning"}) && (v == QLatin1String("on") || v == QLatin1String("off") || v == QLatin1String("auto")))
            valued([&] { ui->reasoningCombo->setCurrentIndex(v == QLatin1String("on") ? ReasoningOn
                                                             : v == QLatin1String("off") ? ReasoningOff : ReasoningAuto); });
        else if (is({"--reasoning-budget"}) && isInt)
            valued([&] { ui->reasoningUnlimitedCheck->setChecked(iv < 0); if (iv >= 0) ui->reasoningBudgetSpin->setValue(iv); });
        else if (is({"--jinja"})) flag(ui->jinjaCheck, true);
        else if (is({"--no-jinja"})) flag(ui->jinjaCheck, false);
        else if (is({"--temp", "--temperature"}) && isNum)
            valued([&] { ui->tempCheck->setChecked(true); ui->tempSpin->setValue(dv); });
        else if (is({"--top-p"}) && isNum) valued([&] { ui->topPCheck->setChecked(true); ui->topPSpin->setValue(dv); });
        else if (is({"--top-k"}) && isInt) valued([&] { ui->topKCheck->setChecked(true); ui->topKSpin->setValue(iv); });
        else if (is({"--min-p"}) && isNum) valued([&] { ui->minPCheck->setChecked(true); ui->minPSpin->setValue(dv); });
        else if (is({"--presence-penalty"}) && isNum && dv >= -2 && dv <= 2)
            valued([&] { ui->presencePenaltyCheck->setChecked(true); ui->presencePenaltySpin->setValue(dv); });
        else if (is({"--frequency-penalty"}) && isNum && dv >= -2 && dv <= 2)
            valued([&] { ui->frequencyPenaltyCheck->setChecked(true); ui->frequencyPenaltySpin->setValue(dv); });
        else if (is({"--metrics"})) flag(ui->metricsCheck, true);
        else if (is({"--no-warmup"})) flag(ui->noWarmupCheck, true);
        else if (is({"--warmup"})) flag(ui->noWarmupCheck, false);
        else if (is({"--context-shift"})) flag(ui->ctxShiftCheck, true);
        else if (is({"--no-context-shift"})) flag(ui->ctxShiftCheck, false);
        else if (is({"--no-mmproj-offload"})) flag(ui->mmprojCpuCheck, true);
        else if (is({"--mmproj-offload"})) flag(ui->mmprojCpuCheck, false);
        else rest << a;
    }

    // One combo sets both K and V, so only an identical pair of a type it offers can move.
    const int kv = kKvCacheTypes.indexOf(kvK);
    if (!kvTokens.isEmpty() && kvK == kvV && kv >= 0) {
        ui->kvCacheCombo->setCurrentIndex(kv);
        moved += 2;
    } else {
        rest << kvTokens;
    }
    if (batch || ubatch) {
        ui->batchAutoCheck->setChecked(false);
        if (batch)
            ui->batchSpin->setValue(batch); // first: it bounds the micro-batch
        if (ubatch)
            ui->ubatchSpin->setValue(ubatch);
    }

    if (moved > 0) {
        ui->extraArgsEdit->setText(joinCommand(rest));
        statusBar()->showMessage(tr("Moved %n option(s) from Extra Arguments to the Advanced tab", nullptr, moved), 5000);
    }
    return moved;
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("serverPath", ui->serverPathEdit->text().trimmed());
    settings.setValue("port", ui->portSpin->value());
    settings.setValue("noUi", ui->noUiCheck->isChecked());
    settings.setValue("lanAccess", ui->lanAccessCheck->isChecked());
    settings.setValue("apiKey", ui->apiKeyEdit->text().trimmed());
    settings.setValue("https", ui->httpsCheck->isChecked());
    settings.setValue("tlsCertFile", ui->certFileEdit->text().trimmed());
    settings.setValue("tlsKeyFile", ui->keyFileEdit->text().trimmed());
    settings.setValue("modelsDir", ui->modelsDirEdit->text().trimmed());
    settings.setValue("lastModel", m_currentModelPath);
    settings.setValue("autoStartServer", ui->autoStartServerCheck->isChecked());
    settings.setValue("notifications", ui->notificationsCheck->isChecked());
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("currentTab", ui->tabWidget->currentIndex());
    if (!m_currentModelPath.isEmpty())
        saveModelSettings(m_currentModelPath);

    // The file may hold the API key: keep it readable by the owner only.
    settings.sync();
    QFile::setPermissions(settings.fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void MainWindow::loadSettings()
{
    migrateLegacySettings();
    m_loading = true;
    QSettings settings;

    ui->serverPathEdit->setText(settings.value("serverPath").toString());
    ui->portSpin->setValue(settings.value("port", 8080).toInt());
    ui->noUiCheck->setChecked(settings.value("noUi", false).toBool());
    ui->lanAccessCheck->setChecked(settings.value("lanAccess", false).toBool());
    ui->apiKeyEdit->setText(settings.value("apiKey").toString());
    ui->httpsCheck->setChecked(settings.value("https", false).toBool() || ui->lanAccessCheck->isChecked());
    ui->certFileEdit->setText(settings.value("tlsCertFile").toString());
    ui->keyFileEdit->setText(settings.value("tlsKeyFile").toString());
    updateNetworkHint();
    ui->autoStartServerCheck->setChecked(settings.value("autoStartServer", false).toBool());
    ui->notificationsCheck->setChecked(settings.value("notifications", true).toBool());
    ui->launchAtLoginCheck->setChecked(QFileInfo::exists(autostartFilePath()));
    // "windowGeometry" belonged to the single-page layout, which was much taller.
    settings.remove("windowGeometry");
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    ui->tabWidget->setCurrentIndex(qBound(0, settings.value("currentTab", 0).toInt(), ui->tabWidget->count() - 1));

    // Older versions kept one global set of parameters (and a single "modelPath");
    // use them as the starting point for models that have no settings of their own yet.
    const QString contextSize = settings.value("contextSize").toString();
    if (!contextSize.isEmpty())
        ui->contextSizeCombo->setCurrentText(contextSize);
    ui->gpuLayersSpin->setValue(settings.value("gpuLayers", 35).toInt());
    ui->gpuAutoCheck->setChecked(settings.value("gpuAuto", true).toBool());
    ui->contextAutoCheck->setChecked(settings.value("contextAuto", true).toBool());
    ui->ctxShiftCheck->setChecked(settings.value("contextShift", false).toBool());
    ui->extraArgsEdit->setText(settings.value("extraArgs").toString());
    ui->flashAttnCombo->setCurrentIndex(qBound(0, settings.value("flashAttn", 0).toInt(), 2));
    m_savedDevice = settings.value("device").toString();

    const QString lastModel = settings.value("lastModel", settings.value("modelPath")).toString();
    QString modelsDir = settings.value("modelsDir").toString();
    if (modelsDir.isEmpty() && !lastModel.isEmpty())
        modelsDir = QFileInfo(lastModel).absolutePath();
    ui->modelsDirEdit->setText(modelsDir);

    m_loading = false;
    rescanModels(lastModel);
}
