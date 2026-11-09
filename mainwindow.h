#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gguf.h"

#include <QMainWindow>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool hasTray() const { return m_trayIcon && m_trayIcon->isVisible(); }

public slots:
    void startServer();
    void stopServer();

private slots:
    void on_browseButton_clicked();
    void on_serverBrowseButton_clicked();
    void on_refreshDevicesButton_clicked();
    void on_modelsDirButton_clicked();
    void on_startButton_clicked();
    void on_openBrowserButton_clicked();
    void on_copyUrlButton_clicked();
    void on_generateKeyButton_clicked();
    void on_copyLogButton_clicked();
    void on_clearLogButton_clicked();
    void updateNetworkHint();
    void readProcessOutput();
    void handleProcessStarted();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onModelChanged(int index);
    void onLaunchAtLoginToggled(bool enabled);
    void saveSettings();
    void loadSettings();
    void updateUiState();
    void updateFitWarning();
    void checkServerHealth();
    void toggleWindowVisibility();
    void quitApplication();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class Status { Stopped, Starting, Loading, Running };

    Ui::MainWindow *ui;
    QProcess *m_process;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_openAction = nullptr;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_healthTimer;
    QTimer *m_killTimer;         // force-kills the server if it ignores SIGTERM

    Status m_status = Status::Stopped;
    QString m_runningModelName;
    int m_runningPort = 8080;
    bool m_runningWebUi = true;
    bool m_runningLan = false;
    bool m_stopRequested = false; // distinguishes a user stop from a crash
    QByteArray m_outputBuffer;    // incomplete trailing line of server output
    QString m_lastOutputLine;

    QString m_currentModelPath;  // model whose per-model settings are shown in the UI
    GgufInfo m_modelInfo;
    QString m_savedDevice;       // device id to select once the device list is loaded
    int m_probeGeneration = 0;
    bool m_loading = false;      // suppresses change handlers while settings are applied

    void setupSystemTray();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void setStatus(Status status);
    void notify(const QString &title, const QString &message,
                QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
    QUrl serverUrl() const;
    QUrl lanUrl() const;          // first LAN address, or empty if not listening on the network
    static QString lanAddress();
    QStringList buildArguments() const;

    QString serverExecutable() const;
    void refreshServerInfo();
    void runServerProbe(const QStringList &args, std::function<void(bool ok, const QString &output)> onDone);
    void populateDevices(const QString &listOutput);
    void selectDevice(const QString &id);

    void rescanModels(const QString &selectPath = QString());
    void addRecentModel(const QString &path);
    void updateModelInfo();
    void loadModelSettings(const QString &modelPath);
    void saveModelSettings(const QString &modelPath);

    static QString autostartFilePath();
    static void migrateLegacySettings();
};
#endif // MAINWINDOW_H
