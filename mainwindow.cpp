#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QIntValidator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_process(new QProcess(this))
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_startAction(nullptr)
    , m_stopAction(nullptr)
    , m_showAction(nullptr)
    , m_exitAction(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_healthTimer(new QTimer(this))
    , m_currentStatus(tr("Stopped"))
    , m_currentModel(tr("None"))
{
    ui->setupUi(this);

    ui->contextSizeCombo->setValidator(new QIntValidator(1, 1000000, this));

    connect(ui->gpuAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);
    connect(ui->contextAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MainWindow::readProcessOutput);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::handleProcessError);
    connect(m_process, &QProcess::finished, this, &MainWindow::handleProcessFinished);
    connect(m_process, &QProcess::started, this, &MainWindow::updateTrayMenuState);

    connect(m_healthTimer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    m_healthTimer->setInterval(2000); // Check every 2 seconds

    connect(ui->serverPathEdit, &QLineEdit::editingFinished, this, &MainWindow::refreshServerInfo);

    loadSettings();
    setupSystemTray();
    refreshServerInfo();
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }
    delete ui;
}

void MainWindow::on_browseButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Model"), "", tr("GGUF Models (*.gguf);;All Files (*)"));
    if (!fileName.isEmpty()) {
        ui->modelPathEdit->setText(fileName);
    }
}

void MainWindow::on_serverBrowseButton_clicked()
{
    const QString current = serverExecutable();
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select llama-server"),
                                                          QFileInfo(current).absolutePath());
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
        return path;
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
    ui->serverVersionLabel->setText(tr("Checking %1...").arg(exe));
    ui->refreshDevicesButton->setEnabled(false);

    runServerProbe({QStringLiteral("--version")}, [this, exe](bool ok, const QString &out) {
        const QRegularExpression re(QStringLiteral("^version:\\s*(.+)$"), QRegularExpression::MultilineOption);
        const auto m = re.match(out);
        if (ok && m.hasMatch())
            ui->serverVersionLabel->setText(tr("%1 — version %2").arg(exe, m.captured(1).trimmed()));
        else
            ui->serverVersionLabel->setText(tr("<span style='color:#c0392b'>Cannot run %1</span>").arg(exe.toHtmlEscaped()));
    });

    runServerProbe({QStringLiteral("--list-devices")}, [this](bool ok, const QString &out) {
        populateDevices(ok ? out : QString());
        ui->refreshDevicesButton->setEnabled(true);
    });
}

void MainWindow::populateDevices(const QString &listOutput)
{
    ui->deviceCombo->clear();
    ui->deviceCombo->addItem(tr("All GPUs (auto)"), QString());

    // Output format:  "  Vulkan1: AMD Radeon Pro WX Series (RADV POLARIS11) (4096 MiB, 4090 MiB free)"
    const QRegularExpression re(QStringLiteral("^\\s+([A-Za-z]+\\d+):\\s+(.+)$"), QRegularExpression::MultilineOption);
    auto it = re.globalMatch(listOutput.section(QStringLiteral("Available devices:"), 1));
    while (it.hasNext()) {
        const auto m = it.next();
        ui->deviceCombo->addItem(QStringLiteral("%1: %2").arg(m.captured(1), m.captured(2).trimmed()), m.captured(1));
    }
    ui->deviceCombo->addItem(tr("CPU only"), QStringLiteral("none"));

    int idx = ui->deviceCombo->findData(m_savedDevice);
    if (idx < 0) {
        // Keep a saved device that is not detected right now instead of silently dropping it.
        ui->deviceCombo->addItem(tr("%1 (not detected)").arg(m_savedDevice), m_savedDevice);
        idx = ui->deviceCombo->count() - 1;
    }
    ui->deviceCombo->setCurrentIndex(idx);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_isQuitting && m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::setupSystemTray()
{
    m_trayMenu = new QMenu(this);

    m_startAction = m_trayMenu->addAction(tr("Start Server"), this, &MainWindow::startServer);
    m_stopAction = m_trayMenu->addAction(tr("Stop Server"), this, &MainWindow::stopServer);
    m_trayMenu->addSeparator();
    m_showAction = m_trayMenu->addAction(tr("Show/Restore"), this, &MainWindow::toggleWindowVisibility);
    m_trayMenu->addSeparator();
    m_exitAction = m_trayMenu->addAction(tr("Exit"), this, &MainWindow::quitApplication);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/rsc/gemini-svg.svg"));
    m_trayIcon->setToolTip(tr("Llama Launcher"));
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    updateTrayMenuState();
    m_trayIcon->show();
    updateUiState();
}

void MainWindow::updateTrayMenuState()
{
    bool isRunning = (m_process->state() != QProcess::NotRunning);
    if (m_startAction) m_startAction->setEnabled(!isRunning);
    if (m_stopAction) m_stopAction->setEnabled(isRunning);
    if (ui->startButton) ui->startButton->setText(isRunning ? tr("Stop Llama Server") : tr("Start Llama Server"));
}

void MainWindow::startServer()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    QString modelPath = ui->modelPathEdit->text();
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a model path first."));
        return;
    }

    QStringList arguments;
    arguments << "--model" << modelPath;

    // GPU Layers
    if (ui->gpuAutoCheck->isChecked()) {
        arguments << "-ngl" << "auto";
    } else {
        arguments << "-ngl" << QString::number(ui->gpuLayersSpin->value());
    }

    // Context Size
    if (ui->contextAutoCheck->isChecked()) {
        arguments << "-c" << "0";
    } else {
        arguments << "-c" << ui->contextSizeCombo->currentText();
    }

    // Fit parameter
    if (ui->gpuAutoCheck->isChecked() || ui->contextAutoCheck->isChecked()) {
        arguments << "--fit" << "on";
    } else {
        arguments << "--fit" << "off";
    }

    if (ui->ctxShiftCheck->isChecked()) {
        arguments << "--context-shift";
    }

    if (ui->noUiCheck->isChecked()) {
        arguments << "--no-ui";
    }

    const QString device = ui->deviceCombo->currentData().toString();
    if (!device.isEmpty()) {
        arguments << "--device" << device;
    }

    static const char *const flashAttnModes[] = {nullptr, "on", "off"}; // index 0 = Auto: leave server default
    const int fa = ui->flashAttnCombo->currentIndex();
    if (fa > 0 && fa < 3) {
        arguments << "-fa" << flashAttnModes[fa];
    }

    arguments << "--port" << QString::number(ui->portSpin->value());

    // Extra user-supplied arguments (appended last so they can override defaults)
    const QString extraArgs = ui->extraArgsEdit->text().trimmed();
    if (!extraArgs.isEmpty()) {
        arguments << QProcess::splitCommand(extraArgs);
    }

    ui->consoleOutput->clear();
    const QString exe = serverExecutable();
    ui->consoleOutput->appendPlainText(tr("Starting %1 with arguments: %2").arg(exe, arguments.join(" ")));

    m_process->start(exe, arguments);

    if (!m_process->waitForStarted()) {
        ui->consoleOutput->appendPlainText(tr("Failed to start process!"));
    } else {
        m_healthTimer->start();
        m_currentStatus = tr("Loading");
        m_currentModel = modelPath.section('/', -1);
        m_currentPort = ui->portSpin->value();
    }
    updateTrayMenuState();
    updateUiState();
    updateTrayTooltip();
}

void MainWindow::stopServer()
{
    if (m_process->state() != QProcess::NotRunning) {
        ui->consoleOutput->appendPlainText(tr("Stopping server..."));
        m_process->terminate();
    }
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

void MainWindow::quitApplication()
{
    m_isQuitting = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }
    qApp->quit();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        toggleWindowVisibility();
    }
}

void MainWindow::readProcessOutput() {
    auto process = qobject_cast<QProcess *>(sender());
    if (!process) return;

    QByteArray output = process->readAllStandardOutput();
    if (!output.isEmpty()) {
        ui->consoleOutput->appendPlainText(QString::fromUtf8(output).trimmed());
    }

    QByteArray errorOutput = process->readAllStandardError();
    if (!errorOutput.isEmpty()) {
        ui->consoleOutput->appendPlainText(QString::fromUtf8(errorOutput).trimmed());
    }
}

void MainWindow::handleProcessError(QProcess::ProcessError error) {
    ui->consoleOutput->appendPlainText(tr("Process error occurred: %1").arg(error));
    updateTrayMenuState();
    m_healthTimer->stop();
    m_currentStatus = tr("Stopped");
    m_currentModel = tr("None");
    updateUiState();
    updateTrayTooltip();
}

void MainWindow::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    ui->consoleOutput->appendPlainText(tr("Process finished with exit code %1 and status %2").arg(exitCode).arg(exitStatus));
    updateTrayMenuState();
    m_healthTimer->stop();
    m_currentStatus = tr("Stopped");
    m_currentModel = tr("None");
    updateUiState();
    updateTrayTooltip();
}

void MainWindow::migrateLegacySettings() {
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

void MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue("serverPath", ui->serverPathEdit->text().trimmed());
    settings.setValue("device", ui->deviceCombo->count() > 0 ? ui->deviceCombo->currentData().toString() : m_savedDevice);
    settings.setValue("flashAttn", ui->flashAttnCombo->currentIndex());
    settings.setValue("modelPath", ui->modelPathEdit->text());
    settings.setValue("contextSize", ui->contextSizeCombo->currentText());
    settings.setValue("gpuLayers", ui->gpuLayersSpin->value());
    settings.setValue("gpuAuto", ui->gpuAutoCheck->isChecked());
    settings.setValue("contextAuto", ui->contextAutoCheck->isChecked());
    settings.setValue("contextShift", ui->ctxShiftCheck->isChecked());
    settings.setValue("noUi", ui->noUiCheck->isChecked());
    settings.setValue("port", ui->portSpin->value());
    settings.setValue("extraArgs", ui->extraArgsEdit->text());
}

void MainWindow::loadSettings() {
    migrateLegacySettings();
    QSettings settings;
    ui->serverPathEdit->setText(settings.value("serverPath").toString());
    m_savedDevice = settings.value("device").toString();
    ui->flashAttnCombo->setCurrentIndex(qBound(0, settings.value("flashAttn", 0).toInt(), 2));
    ui->modelPathEdit->setText(settings.value("modelPath").toString());

    QString contextSize = settings.value("contextSize").toString();
    if (!contextSize.isEmpty()) {
        ui->contextSizeCombo->setCurrentText(contextSize);
    }

    ui->gpuLayersSpin->setValue(settings.value("gpuLayers", 35).toInt());
    ui->gpuAutoCheck->setChecked(settings.value("gpuAuto", false).toBool());
    ui->contextAutoCheck->setChecked(settings.value("contextAuto", false).toBool());

    updateUiState();

    ui->ctxShiftCheck->setChecked(settings.value("contextShift", false).toBool());
    ui->noUiCheck->setChecked(settings.value("noUi", false).toBool());
    ui->portSpin->setValue(settings.value("port", 8080).toInt());
    ui->extraArgsEdit->setText(settings.value("extraArgs").toString());
}

void MainWindow::updateUiState() {
    ui->gpuLayersSpin->setDisabled(ui->gpuAutoCheck->isChecked());
    ui->contextSizeCombo->setDisabled(ui->contextAutoCheck->isChecked());
}

void MainWindow::on_startButton_clicked()
{
    if (m_process->state() != QProcess::NotRunning) {
        stopServer();
    } else {
        startServer();
    }
}

void MainWindow::updateTrayTooltip() {
    if (!m_trayIcon) return;
    QString tooltip = tr("Llama Launcher\nStatus: %1\nModel: %2")
                          .arg(m_currentStatus)
                          .arg(m_currentModel);
    m_trayIcon->setToolTip(tooltip);
}

void MainWindow::checkServerHealth() {
    if (m_process->state() == QProcess::NotRunning) {
        m_healthTimer->stop();
        m_currentStatus = tr("Stopped");
        m_currentModel = tr("None");
        updateTrayTooltip();
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(m_currentPort)));
    request.setTransferTimeout(1500); // shorter than the 2 s poll interval, so replies don't pile up
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 200) {
                m_currentStatus = tr("Running");
            } else if (statusCode == 503) {
                m_currentStatus = tr("Loading");
            }
        } else {
            // If the server is not reachable yet, it might still be starting up
            if (m_process->state() != QProcess::NotRunning) {
                m_currentStatus = tr("Loading");
            } else {
                m_currentStatus = tr("Stopped");
                m_currentModel = tr("None");
            }
        }
        updateTrayTooltip();
        reply->deleteLater();
    });
}

