#include "widget.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    if(translator.load(":/ChatProgramForClient_ko_KR.qm"))
        QApplication::installTranslator(&translator);

    Widget w;
    w.show();
    return a.exec();
}
