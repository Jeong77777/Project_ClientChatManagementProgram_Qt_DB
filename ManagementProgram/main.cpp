#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    if(translator.load(":/ManagementProgram_ko_KR.qm"))
        QApplication::installTranslator(&translator);

    MainWindow w;
    w.show();
    return a.exec();
}
