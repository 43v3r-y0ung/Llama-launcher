#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTranslator>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace {

// SIGTERM (logout, kill), SIGINT and SIGHUP would end the process without stopping the
// server or saving settings. The handler only writes a byte to a socket pair; the event loop
// picks it up and quits the normal way.
int g_signalFds[2] = {-1, -1};

void onQuitSignal(int)
{
    const char byte = 1;
    [[maybe_unused]] const ssize_t written = ::write(g_signalFds[0], &byte, 1);
}

void installQuitSignalHandlers(MainWindow *window)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, g_signalFds) != 0)
        return;
    auto *notifier = new QSocketNotifier(g_signalFds[1], QSocketNotifier::Read, window);
    QObject::connect(notifier, &QSocketNotifier::activated, window, [window, notifier] {
        notifier->setEnabled(false);
        char byte;
        [[maybe_unused]] const ssize_t got = ::read(g_signalFds[1], &byte, 1);
        window->quitApplication();
    });

    struct sigaction action = {};
    action.sa_handler = onQuitSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    for (int sig : {SIGTERM, SIGINT, SIGHUP})
        ::sigaction(sig, &action, nullptr);
}

QString instanceSocketName()
{
    // Per-user runtime directory, so other users on the machine have their own launcher.
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return runtimeDir.isEmpty() ? QStringLiteral("llama-launcher") : runtimeDir + QStringLiteral("/llama-launcher.sock");
}

// Asks a launcher that is already running to show its window. Returns false if there is none.
bool activateRunningInstance(const QString &socketName, bool showWindow)
{
    QLocalSocket socket;
    socket.connectToServer(socketName);
    if (!socket.waitForConnected(1000))
        return false;
    if (showWindow) {
        socket.write("show\n");
        socket.waitForBytesWritten(1000);
    }
    socket.disconnectFromServer();
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("LlamaLauncher"));
    QApplication::setApplicationName(QStringLiteral("LlamaLauncher"));
    QApplication::setApplicationDisplayName(QStringLiteral("Llama Launcher"));
    QApplication::setDesktopFileName(QStringLiteral("llama-launcher"));
    a.setWindowIcon(QIcon(":/rsc/llama-launcher.svg"));
    // Closing the window only hides it to the tray; quitting is explicit.
    QApplication::setQuitOnLastWindowClosed(false);

    // UI language follows the system locale; Qt's own dialogs too.
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&qtTranslator);
    QTranslator appTranslator;
    if (appTranslator.load(QLocale(), QStringLiteral("llama-launcher"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QApplication::installTranslator(&appTranslator);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption autostartOption(QStringLiteral("autostart"),
                                       QApplication::translate("main", "Start hidden in the system tray (used at login)."));
    parser.addOption(autostartOption);
    parser.process(a);

    // A second launcher would start a second server on the same port; bring up the first one instead.
    // At login (--autostart) there is nothing to show, the running launcher is left alone.
    const QString socketName = instanceSocketName();
    if (activateRunningInstance(socketName, !parser.isSet(autostartOption)))
        return 0;

    MainWindow w;
    installQuitSignalHandlers(&w);

    QLocalServer instanceServer;
    instanceServer.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(socketName); // left behind by a launcher that was killed
    if (instanceServer.listen(socketName)) {
        QObject::connect(&instanceServer, &QLocalServer::newConnection, &w, [&instanceServer, &w] {
            while (QLocalSocket *client = instanceServer.nextPendingConnection()) {
                QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
                const auto readCommands = [client, &w] {
                    while (client->canReadLine()) {
                        if (client->readLine().trimmed() == "show")
                            w.bringToFront();
                    }
                };
                QObject::connect(client, &QLocalSocket::readyRead, &w, readCommands);
                readCommands(); // the command may have arrived before the connection was picked up
            }
        });
    } else {
        qWarning("Cannot listen on %s: %s", qPrintable(socketName), qPrintable(instanceServer.errorString()));
    }
    // At login, stay in the tray, unless there is no tray to come back from.
    if (!parser.isSet(autostartOption) || !w.hasTray())
        w.show();
    return QApplication::exec();
}
