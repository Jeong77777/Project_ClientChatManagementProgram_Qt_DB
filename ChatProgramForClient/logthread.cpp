#include "logthread.h"

#include <QFile>
#include <QTextStream>

/**
 * @brief 생성자, 로그를 저장하는 파일 이름 설정
 */
LogThread::LogThread(int id, std::string name, QObject *parent)
    : QThread{parent}, m_filename("")
{
    m_filename = "log_" + std::to_string(id) + "_" + name + ".txt";
}

/**
 * @brief 로그를 1분마다 자동으로 저장
 */
void LogThread::run()
{
    while(true) {
        saveData();
        sleep(60);      // 1분마다 저장
    }
}

/**
 * @brief 새로운 채팅 추가
 * @Param std::string data 채팅 내용
 */
void LogThread::appendData(const std::string data)
{
    m_chatList.push_back(data);
}

/**
 * @brief 채팅 로그 저장
 */
void LogThread::saveData()
{
    if(m_chatList.size() > 0) {
        QFile file(QString::fromStdString(m_filename));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
            return;

        QTextStream out(&file);
        for(const auto& data : m_chatList) {
            out << QString::fromStdString(data) << "\n";
        }
        file.close();
        m_chatList.clear();
    }
}
