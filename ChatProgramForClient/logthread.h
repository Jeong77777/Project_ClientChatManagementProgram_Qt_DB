#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <string>
#include <vector>

/**
 * @brief 채팅 메시지 로그를 기록 thread
 */
class LogThread : public QThread
{
    Q_OBJECT
public:
    explicit LogThread(int id = 0, std::string name = "",
                       QObject *parent = nullptr);

private:
    void run() override;                     // 1분마다 채팅 로그를 저장

    std::vector<std::string> m_chatList;  // 채팅 내용을 저장하는 list
    std::string m_filename;               // 로그를 저장하는 파일의 이름

public slots:
    void appendData(const std::string);       // 새 채팅 내용 추가
    void saveData();                // 채팅 로그 저장
};

#endif // LOGTHREAD_H
