#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIntValidator>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

namespace {

constexpr int kMaxRecentModels = 10;
constexpr int kMaxConsoleLines = 5000;
constexpr int kKillTimeoutMs = 5000;
constexpr int kDeviceFreeMiBRole = Qt::UserRole + 1;

enum MoeMode { MoeAuto = 0, MoeAll = 1, MoeFirstN = 2 };
enum FlashAttn { FaAuto = 0, FaOn = 1, FaOff = 2 };

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
    , m_process(new QProcess(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_healthTimer(new QTimer(this))
    , m_killTimer(new QTimer(this))
{
    ui->setupUi(this);

    ui->contextSizeCombo->setValidator(new QIntValidator(1, 10000000, this));
    ui->consoleOutput->setMaximumBlockCount(kMaxConsoleLines);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->consoleOutput->setFont(mono);
    ui->openBrowserButton->setIcon(QIcon::fromTheme(QStringLiteral("internet-web-browser")));
    ui->copyUrlButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    ui->refreshDevicesButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));

    connect(ui->gpuAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);
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

    // Server output: stdout and stderr interleaved in the order they were written.
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(m_process, &QProcess::started, this, &MainWindow::handleProcessStarted);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::handleProcessError);
    connect(m_process, &QProcess::finished, this, &MainWindow::handleProcessFinished);

    connect(m_healthTimer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    m_healthTimer->setInterval(2000);

    m_killTimer->setSingleShot(true);
    m_killTimer->setInterval(kKillTimeoutMs);
    connect(m_killTimer, &QTimer::timeout, this, [this] {
        if (m_process->state() != QProcess::NotRunning) {
            ui->consoleOutput->appendPlainText(tr("Server did not stop in time, killing it."));
            m_process->kill();
        }
    });

    setupSystemTray();
    loadSettings();
    setStatus(Status::Stopped);
    refreshServerInfo();

    if (ui->autoStartServerCheck->isChecked())
        QTimer::singleShot(0, this, &MainWindow::startServer);
}

MainWindow::~MainWindow()
{
    saveSettings();

    // Child processes (the server and any --version/--list-devices probes) emit finished()
    // when they are destroyed; make sure none of that reaches handlers that use `ui`.
    const auto processes = findChildren<QProcess *>();
    for (QProcess *process : processes)
        process->disconnect(this);

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }
    delete ui;
}

// ---------------------------------------------------------------------------
// Window & tray
// ---------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (hasTray()) {
        saveSettings();
        hide();
        event->ignore();
    } else {
        quitApplication();
        event->accept();
    }
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
    m_trayMenu->addAction(tr("Exit"), this, &MainWindow::quitApplication);

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
        show();
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        toggleWindowVisibility();
}

void MainWindow::quitApplication()
{
    saveSettings();
    if (m_process->state() != QProcess::NotRunning) {
        m_stopRequested = true;
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }
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
    return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(m_runningPort));
}

QString MainWindow::lanAddress()
{
    // Pick the IPv4 address of a real network card; skip loopback and virtual bridges
    // (Docker, libvirt, VPN tunnels...) whose addresses are useless to other machines.
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
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback() && !ip.isLinkLocal())
                return ip.toString();
        }
    }
    return {};
}

QUrl MainWindow::lanUrl() const
{
    const QString ip = m_runningLan ? lanAddress() : QString();
    return ip.isEmpty() ? QUrl() : QUrl(QStringLiteral("http://%1:%2/").arg(ip).arg(m_runningPort));
}

void MainWindow::setStatus(Status status)
{
    const Status previous = m_status;
    m_status = status;
    const bool running = m_process->state() != QProcess::NotRunning;

    QString text, color;
    switch (status) {
    case Status::Stopped:  text = tr("Stopped");  color = QStringLiteral("#7f8c8d"); break;
    case Status::Starting: text = tr("Starting"); color = QStringLiteral("#e67e22"); break;
    case Status::Loading:  text = tr("Loading model"); color = QStringLiteral("#e67e22"); break;
    case Status::Running:  text = tr("Running");  color = QStringLiteral("#27ae60"); break;
    }

    QString html = QStringLiteral("<span style='color:%1'>&#9679;</span> <b>%2</b>").arg(color, text);
    if (status != Status::Stopped) {
        const QString url = serverUrl().toString();
        html += QStringLiteral(" &mdash; %1 &mdash; <a href='%2'>%2</a>")
                    .arg(m_runningModelName.toHtmlEscaped(), url.toHtmlEscaped());
        const QString lan = lanUrl().toString();
        if (!lan.isEmpty())
            html += QStringLiteral(" &middot; %1 <a href='%2'>%2</a>").arg(tr("network:"), lan.toHtmlEscaped());
    }
    ui->statusLabel->setText(html);
    ui->statusLabel->setOpenExternalLinks(true);

    const bool ready = status == Status::Running;
    ui->openBrowserButton->setEnabled(ready && m_runningWebUi);
    ui->copyUrlButton->setEnabled(status != Status::Stopped);
    ui->startButton->setText(running ? tr("Stop Llama Server") : tr("Start Llama Server"));

    if (m_trayIcon) {
        m_startAction->setEnabled(!running);
        m_stopAction->setEnabled(running);
        m_openAction->setEnabled(ready && m_runningWebUi);
        QString tooltip = tr("Llama Launcher\nStatus: %1").arg(text);
        if (status != Status::Stopped)
            tooltip += tr("\nModel: %1\n%2").arg(m_runningModelName, serverUrl().toString());
        if (status != Status::Stopped && !lanUrl().isEmpty())
            tooltip += QLatin1Char('\n') + lanUrl().toString();
        m_trayIcon->setToolTip(tooltip);
    }

    if (previous != Status::Running && status == Status::Running) {
        const QUrl lan = lanUrl();
        notify(tr("Llama server is ready"),
               tr("%1\n%2").arg(m_runningModelName, (lan.isEmpty() ? serverUrl() : lan).toString()));
    }

    updateUiState();
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
    QString html;
    if (lan) {
        const QString ip = lanAddress();
        const QString addr = ip.isEmpty() ? tr("no network connection found")
                                          : QStringLiteral("http://%1:%2/").arg(ip).arg(ui->portSpin->value());
        html = tr("Other computers can connect at %1").arg(QStringLiteral("<b>%1</b>").arg(addr.toHtmlEscaped()));
        if (!hasKey)
            html += QStringLiteral("<br><span style='color:#d35400'>&#9888; %1</span>")
                        .arg(tr("No API key: anyone on the network can use the server. Set a key, especially on public Wi-Fi.").toHtmlEscaped());
    }
    ui->networkHintLabel->setText(html);
    ui->networkHintLabel->setVisible(!html.isEmpty());
}

// ---------------------------------------------------------------------------
// Server process
// ---------------------------------------------------------------------------

void MainWindow::on_startButton_clicked()
{
    if (m_process->state() != QProcess::NotRunning)
        stopServer();
    else
        startServer();
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

    // --fit adjusts only the parameters left on "auto"; with everything fixed, turn it off.
    const bool anyAuto = ui->gpuAutoCheck->isChecked() || ui->contextAutoCheck->isChecked();
    arguments << "--fit" << (anyAuto ? "on" : "off");

    const QString device = ui->deviceCombo->currentData().toString();
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
    if (ui->noUiCheck->isChecked())
        arguments << "--no-ui";
    if (ui->lanAccessCheck->isChecked())
        arguments << "--host" << "0.0.0.0";
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
    if (m_process->state() != QProcess::NotRunning)
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

    saveSettings(); // persist what is about to be used, even if the session ends abruptly
    addRecentModel(modelPath);

    const QStringList arguments = buildArguments();
    const QString exe = serverExecutable();

    m_stopRequested = false;
    m_outputBuffer.clear();
    m_lastOutputLine.clear();
    m_runningModelName = QFileInfo(modelPath).fileName();
    m_runningPort = ui->portSpin->value();
    m_runningWebUi = !ui->noUiCheck->isChecked();
    m_runningLan = ui->lanAccessCheck->isChecked();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("LLAMA_API_KEY"));
    const QString apiKey = ui->apiKeyEdit->text().trimmed();
    if (!apiKey.isEmpty())
        env.insert(QStringLiteral("LLAMA_API_KEY"), apiKey);
    m_process->setProcessEnvironment(env);

    ui->consoleOutput->clear();
    ui->consoleOutput->appendPlainText(tr("Starting %1 %2").arg(exe, arguments.join(QLatin1Char(' '))));
    setStatus(Status::Starting);
    m_process->start(exe, arguments); // asynchronous: started() / errorOccurred() follow
}

void MainWindow::handleProcessStarted()
{
    setStatus(Status::Loading);
    m_healthTimer->start();
}

void MainWindow::stopServer()
{
    if (m_process->state() == QProcess::NotRunning)
        return;
    ui->consoleOutput->appendPlainText(tr("Stopping server..."));
    m_stopRequested = true;
    m_process->terminate();
    m_killTimer->start();
}

void MainWindow::readProcessOutput()
{
    m_outputBuffer += m_process->readAllStandardOutput();

    // Only print complete lines so chunk boundaries do not split them.
    const int lastNewline = m_outputBuffer.lastIndexOf('\n');
    if (lastNewline < 0)
        return;
    const QString text = QString::fromUtf8(m_outputBuffer.left(lastNewline));
    m_outputBuffer.remove(0, lastNewline + 1);

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        ui->consoleOutput->appendPlainText(trimmed);
        m_lastOutputLine = trimmed;
    }
}

void MainWindow::handleProcessError(QProcess::ProcessError error)
{
    // Crashed is followed by finished(), which reports it; only FailedToStart ends here.
    if (error != QProcess::FailedToStart) {
        if (!(error == QProcess::Crashed && m_stopRequested))
            ui->consoleOutput->appendPlainText(tr("Process error: %1").arg(processErrorText(error)));
        return;
    }

    const QString msg = tr("Could not start %1: %2").arg(serverExecutable(), processErrorText(error));
    ui->consoleOutput->appendPlainText(msg);
    m_healthTimer->stop();
    setStatus(Status::Stopped);
    notify(tr("Llama server failed to start"), msg, QSystemTrayIcon::Critical);
}

void MainWindow::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_killTimer->stop();
    m_healthTimer->stop();
    if (!m_outputBuffer.isEmpty()) {
        ui->consoleOutput->appendPlainText(QString::fromUtf8(m_outputBuffer).trimmed());
        m_outputBuffer.clear();
    }

    const bool unexpected = !m_stopRequested && (exitStatus == QProcess::CrashExit || exitCode != 0);
    if (exitStatus == QProcess::CrashExit && !m_stopRequested)
        ui->consoleOutput->appendPlainText(tr("Server crashed."));
    else
        ui->consoleOutput->appendPlainText(tr("Server stopped (exit code %1).").arg(exitCode));

    setStatus(Status::Stopped);

    if (unexpected) {
        QString msg = tr("Server stopped unexpectedly (exit code %1).").arg(exitCode);
        if (!m_lastOutputLine.isEmpty())
            msg += QLatin1Char('\n') + m_lastOutputLine.right(200);
        notify(tr("Llama server stopped"), msg, QSystemTrayIcon::Warning);
    }
}

void MainWindow::checkServerHealth()
{
    if (m_process->state() == QProcess::NotRunning) {
        m_healthTimer->stop();
        return;
    }

    QNetworkRequest request(serverUrl().resolved(QUrl(QStringLiteral("health"))));
    request.setTransferTimeout(1500); // shorter than the 2 s poll interval, so replies don't pile up
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (m_process->state() == QProcess::NotRunning)
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
    const bool running = m_process->state() != QProcess::NotRunning;

    ui->gpuLayersSpin->setDisabled(ui->gpuAutoCheck->isChecked());
    ui->contextSizeCombo->setDisabled(ui->contextAutoCheck->isChecked());

    // MoE options only make sense for MoE models; keep them enabled if metadata is unknown.
    const bool moeApplicable = !m_modelInfo.valid || m_modelInfo.isMoe();
    ui->moeModeCombo->setEnabled(moeApplicable);
    ui->moeLayersSpin->setEnabled(moeApplicable && ui->moeModeCombo->currentIndex() == MoeFirstN);
    ui->labelMoe->setEnabled(moeApplicable);
    if (m_modelInfo.blockCount)
        ui->moeLayersSpin->setMaximum(int(m_modelInfo.blockCount));

    // Changing the model or binary while the server runs would not affect it; avoid confusion.
    ui->modelGroup->setEnabled(!running);
    ui->paramsGroup->setEnabled(!running);
    ui->deviceCombo->setEnabled(!running);
    ui->refreshDevicesButton->setEnabled(!running);
    ui->portSpin->setEnabled(!running);
    ui->noUiCheck->setEnabled(!running);
    ui->lanAccessCheck->setEnabled(!running);
    ui->apiKeyEdit->setEnabled(!running);
    ui->generateKeyButton->setEnabled(!running);
    ui->serverPathEdit->setEnabled(!running);
    ui->serverBrowseButton->setEnabled(!running);
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
    settings.endGroup();
}

void MainWindow::loadModelSettings(const QString &modelPath)
{
    QSettings settings;
    const QString group = modelSettingsGroup(modelPath);
    if (!settings.contains(group + QStringLiteral("/path"))) {
        return; // first time with this model: keep the current values as its starting point
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
    settings.endGroup();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("serverPath", ui->serverPathEdit->text().trimmed());
    settings.setValue("port", ui->portSpin->value());
    settings.setValue("noUi", ui->noUiCheck->isChecked());
    settings.setValue("lanAccess", ui->lanAccessCheck->isChecked());
    settings.setValue("apiKey", ui->apiKeyEdit->text().trimmed());
    settings.setValue("modelsDir", ui->modelsDirEdit->text().trimmed());
    settings.setValue("lastModel", m_currentModelPath);
    settings.setValue("autoStartServer", ui->autoStartServerCheck->isChecked());
    settings.setValue("notifications", ui->notificationsCheck->isChecked());
    settings.setValue("windowGeometry", saveGeometry());
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
    updateNetworkHint();
    ui->autoStartServerCheck->setChecked(settings.value("autoStartServer", false).toBool());
    ui->notificationsCheck->setChecked(settings.value("notifications", true).toBool());
    ui->launchAtLoginCheck->setChecked(QFileInfo::exists(autostartFilePath()));
    restoreGeometry(settings.value("windowGeometry").toByteArray());

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
