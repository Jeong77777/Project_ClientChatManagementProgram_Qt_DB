#include "logthread.h"

#include <QFile>
#include <QTextStream>

/**
 * @brief 생성자, 로그를 저장하는 파일 이름 설정
 */
LogThread::LogThread(int id, QString name, QObject *parent)
    : QThread{parent}
{
    filename = "log_" + QString::number(id) + "_" + name + ".txt";
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
 * @Param QString data 채팅 내용
 */
void LogThread::appendData(QString data)
{
    chatList.append(data);
}

/**
 * @brief 채팅 로그 저장
 */
void LogThread::saveData()
{
    if(chatList.count() > 0) {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
            return;

        QTextStream out(&file);
        foreach(auto data, chatList) {
            out << data << "\n";
        }
        file.close();
        chatList.clear();
    }
}
