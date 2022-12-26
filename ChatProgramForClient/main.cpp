#include "widget.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication chatProgramForClient(argc, argv);

    QTranslator translator;
    if(translator.load(":/ChatProgramForClient_ko_KR.qm"))
        QApplication::installTranslator(&translator);

    Widget mainWidget;
    mainWidget.show();
    return chatProgramForClient.exec();
}
