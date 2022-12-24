#include "logthread.h"

#include <QTreeWidgetItem>
#include <QFile>
#include <QDateTime>

/**
 * @brief 생성자, 로그를 저장하는 파일 이름 설정
 */
LogThread::LogThread(QObject *parent)
    : QThread{parent}, filename("")
{
    std::string format = "yyyyMMdd_hhmmss";
    filename = std::string("log_.txt")\
            .insert(4, QDateTime::currentDateTime().toString().toStdString());
}

/**
 * @brief 로그를 1분마다 자동으로 저장
 */
void LogThread::run()
{
    Q_FOREVER {
        saveData();
        sleep(60);      // 1분마다 저장
    }
}

/**
 * @brief 새로운 채팅 추가
 * @Param QTreeWidgetItem* item 로그 tree widget item
 */
void LogThread::appendData(QTreeWidgetItem* item)
{
    itemList.push_back(item);
}

/**
 * @brief 채팅 로그 저장
 */
void LogThread::saveData()
{
    if(itemList.size() > 0) {
        QFile file(QString::fromStdString(filename));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);
        foreach(auto item, itemList) {
            out << item->text(0) << " | ";
            out << item->text(1) << " | ";
            out << item->text(2) << " | ";
            out << item->text(3) << " | ";
            out << item->text(4) << " | ";
            out << item->text(5) << "\n";
        }
        file.close();
    }
}
