#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QIntValidator>

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

    loadSettings();
    setupSystemTray();
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

    ui->consoleOutput->clear();
    ui->consoleOutput->appendPlainText(tr("Starting llama-server with arguments: %1").arg(arguments.join(" ")));

    m_process->start("llama-server", arguments);

    if (!m_process->waitForStarted()) {
        ui->consoleOutput->appendPlainText(tr("Failed to start process!"));
    } else {
        m_healthTimer->start();
        m_currentStatus = tr("Loading");
        m_currentModel = modelPath.section('/', -1);
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

void MainWindow::saveSettings() {
    QSettings settings("MyCompany", "LlamaLauncher");
    settings.setValue("modelPath", ui->modelPathEdit->text());
    settings.setValue("contextSize", ui->contextSizeCombo->currentText());
    settings.setValue("gpuLayers", ui->gpuLayersSpin->value());
    settings.setValue("gpuAuto", ui->gpuAutoCheck->isChecked());
    settings.setValue("contextAuto", ui->contextAutoCheck->isChecked());
    settings.setValue("contextShift", ui->ctxShiftCheck->isChecked());
    settings.setValue("noUi", ui->noUiCheck->isChecked());
}

void MainWindow::loadSettings() {
    QSettings settings("MyCompany", "LlamaLauncher");
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

    QNetworkRequest request(QUrl("http://localhost:8080/health"));
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

