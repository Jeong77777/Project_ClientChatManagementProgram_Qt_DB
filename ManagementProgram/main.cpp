#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication managementProgram(argc, argv);

    QTranslator translator;
    if(translator.load(":/ManagementProgram_ko_KR.qm"))
        QApplication::installTranslator(&translator);

    MainWindow mainWindow;
    mainWindow.show();
    return managementProgram.exec();
}
