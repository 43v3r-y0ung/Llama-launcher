#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("LlamaLauncher"));
    QApplication::setApplicationName(QStringLiteral("LlamaLauncher"));
    QApplication::setDesktopFileName(QStringLiteral("llama-launcher"));
    a.setWindowIcon(QIcon(":/rsc/gemini-svg.svg"));
    MainWindow w;
    w.show();
    return QApplication::exec();
}
