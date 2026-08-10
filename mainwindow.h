#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_browseButton_clicked();
    void on_startButton_clicked();
    void readProcessOutput();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void saveSettings();
    void loadSettings();
    void updateUiState();
    void updateTrayMenuState();
    void updateTrayTooltip();
    void checkServerHealth();
    void toggleWindowVisibility();
    void startServer();
    void stopServer();
    void quitApplication();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QProcess *m_process;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_startAction;
    QAction *m_stopAction;
    QAction *m_showAction;
    QAction *m_exitAction;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_healthTimer;
    QString m_currentStatus;
    QString m_currentModel;
    bool m_isQuitting = false;

    void setupSystemTray();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
};
#endif // MAINWINDOW_H
