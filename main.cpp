#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("LlamaLauncher"));
    QApplication::setApplicationName(QStringLiteral("LlamaLauncher"));
    QApplication::setApplicationDisplayName(QStringLiteral("Llama Launcher"));
    QApplication::setDesktopFileName(QStringLiteral("llama-launcher"));
    a.setWindowIcon(QIcon(":/rsc/gemini-svg.svg"));
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

    MainWindow w;
    // At login, stay in the tray, unless there is no tray to come back from.
    if (!parser.isSet(autostartOption) || !w.hasTray())
        w.show();
    return QApplication::exec();
}
