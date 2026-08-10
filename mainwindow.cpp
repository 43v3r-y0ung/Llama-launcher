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
{
    ui->setupUi(this);

    ui->contextSizeCombo->setValidator(new QIntValidator(1, 1000000, this));

    connect(ui->gpuAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);
    connect(ui->contextAutoCheck, &QCheckBox::toggled, this, &MainWindow::updateUiState);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MainWindow::readProcessOutput);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::handleProcessError);
    connect(m_process, &QProcess::finished, this, &MainWindow::handleProcessFinished);

    loadSettings();
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

void MainWindow::on_startButton_clicked()
{
    if (m_process->state() != QProcess::NotRunning) {
        ui->consoleOutput->appendPlainText(tr("Stopping server..."));
        m_process->terminate();
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
        // --fit on is default, but we can be explicit or just omit it.
        // The requirements say: "If at least one of the checkboxes is enabled: add --fit on (or add nothing)"
        // I will add it to be explicit as requested.
        arguments << "--fit" << "on";
    } else {
        // Both off: manual mode
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
        ui->startButton->setText(tr("Stop Llama Server"));
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
    ui->startButton->setText(tr("Start Llama Server"));
}

void MainWindow::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    ui->consoleOutput->appendPlainText(tr("Process finished with exit code %1 and status %2").arg(exitCode).arg(exitStatus));
    ui->startButton->setText(tr("Start Llama Server"));
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

