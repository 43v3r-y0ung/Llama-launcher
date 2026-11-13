#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gguf.h"
#include "logadvisor.h"
#include "monitor.h"
#include "serverprocess.h"

#include <QMainWindow>
#include <QProcess>
#include <QSslCertificate>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QToolButton>
#include <QComboBox>
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
    void cancelServerAutostart() { m_serverAutostartCancelled = true; } // after a launcher restart

public slots:
    void startServer();
    void stopServer();
    void bringToFront();    // show and raise the window, e.g. when the launcher is started again
    void quitApplication(); // stops the server and saves settings before quitting
    void quitKeepingServer(); // saves settings and quits; the next launcher adopts the server
    void restartLauncher();   // runs the launcher binary again (a newer one, if installed); the server keeps running

private slots:
    void on_browseButton_clicked();
    void on_serverBrowseButton_clicked();
    void on_refreshDevicesButton_clicked();
    void on_modelsDirButton_clicked();
    void on_openBrowserButton_clicked();
    void on_copyUrlButton_clicked();
    void on_generateKeyButton_clicked();
    void on_generateCertButton_clicked();
    void on_certBrowseButton_clicked();
    void on_keyBrowseButton_clicked();
    void on_copyLogButton_clicked();
    void on_clearLogButton_clicked();
    void updateNetworkHint();
    void handleServerOutput(const QStringList &lines);
    void handleServerFinished(int exitCode, int crashSignal);
    void onModelChanged(int index);
    void onLaunchAtLoginToggled(bool enabled);
    void saveSettings();
    void loadSettings();
    void updateUiState();
    void updateFitWarning();
    void checkServerHealth();
    void toggleWindowVisibility();
    void toggleServer();
    void restartServer();
    void setWhenIdle(int action); // WhenIdle; toggles the pending action
    void requestQuit();
    void setupCornerControls();
    void setupSamplingPresets();
    void updatePresetCombo();    // selects the preset the sampling values match, if any
    void applySamplingPreset(int index);
    void saveSamplingPreset();
    void deleteSamplingPreset();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    enum class Status { Stopped, Starting, Loading, Running };

    Ui::MainWindow *ui;
    ServerProcess *m_server;
    QString m_launcherPath;      // the launcher binary as started, for restarting it
    bool m_serverAutostartCancelled = false;
    bool m_quitting = false;     // Qt 6 closes the windows on quit(); closeEvent() must let that through
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_openAction = nullptr;
    QToolButton *m_startStopButton = nullptr; // in the corner of the tab bar
    QAction *m_menuStartAction = nullptr;
    QAction *m_menuStopAction = nullptr;
    QAction *m_menuRestartAction = nullptr;
    QAction *m_restartIdleAction = nullptr;
    QToolButton *m_applyButton = nullptr;     // "Restart to Apply", shown while settings differ from the running server
    QTimer *m_pendingTimer = nullptr;
    QComboBox *m_presetCombo = nullptr;
    QToolButton *m_deletePresetButton = nullptr;
    QAction *m_stopIdleAction = nullptr;
    enum WhenIdle { IdleNothing, IdleRestart, IdleStop };
    WhenIdle m_whenIdle = IdleNothing; // done once no request is being processed
    int m_idleSamples = 0;
    int m_busySlots = -1;        // from the latest monitor sample
    QNetworkAccessManager *m_networkManager;
    QTimer *m_healthTimer;
    QTimer *m_killTimer;         // force-kills the server if it ignores SIGTERM
    Monitor *m_monitor;          // CPU/RAM/GPU and llama-server statistics

    Status m_status = Status::Stopped;
    QString m_runningModelName;
    int m_runningPort = 8080;
    bool m_runningWebUi = true;
    bool m_runningLan = false;
    bool m_runningHttps = false;
    QSslCertificate m_runningCert;  // what the HTTPS server presents; the launcher trusts only this
    bool m_stopRequested = false; // distinguishes a user stop from a crash
    QString m_lastOutputLine;
    LogAdvisor m_logAdvisor;     // hints from known server warnings, shown on the Log tab
    QList<LogAdvisor::HintId> m_appliedFixes; // hints whose fix was applied during this run
    bool m_restartPending = false; // start the server again once it has stopped

    QString m_currentModelPath;  // model whose per-model settings are shown in the UI
    GgufInfo m_modelInfo;
    QString m_mmprojPath;        // projector next to the current model, empty if none
    QString m_savedDevice;       // device id to select once the device list is loaded
    QString m_gpuName;           // AMD GPU name as llama-server reports it, for the monitor
    int m_probeGeneration = 0;
    bool m_loading = false;      // suppresses change handlers while settings are applied

    void setupSystemTray();
    bool adoptRunningServer();   // picks up a server left running by a previous launcher
    void serverStarted();        // common to starting and adopting
    void setupCharts();
    void onMonitorSample(const MonitorSample &sample);
    void updateMonitorActivity(); // sample only while someone can see it or the server runs
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void setStatus(Status status);
    void updateStatusText();     // status line and tray tooltip
    void updatePendingChanges(); // compares the settings with what the running server was started with
    void updateLogAdvice();
    void onLogAdviceLinkActivated(const QString &link);
    static QString hintFixLabel(LogAdvisor::HintId id); // empty when the hint has no one-click fix
    void applyHintFix(LogAdvisor::HintId id);
    void notify(const QString &title, const QString &message,
                QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
    QUrl serverUrl() const;
    QUrl lanUrl() const;          // first LAN address, or empty if not listening on the network
    static QString lanAddress();
    static QStringList lanAddresses();
    bool httpsEnabled() const;     // checked, or forced on by network access
    bool usesManagedCert() const;  // no certificate of the user's own: the launcher creates one
    QString tlsCertFile() const;
    QString tlsKeyFile() const;
    bool prepareTls(QString *note, QString *error); // makes sure the certificate exists and fits
    void acceptPinnedCert(QNetworkReply *reply) const;
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
    int migrateExtraArgs();       // moves flags that have their own control out of "Extra Arguments"

    static QString autostartFilePath();
    static void migrateLegacySettings();
};
#endif // MAINWINDOW_H
