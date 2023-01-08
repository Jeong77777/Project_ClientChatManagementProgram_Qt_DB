#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <string>
#include <vector>

class QTreeWidgetItem;

/**
 * @brief 채팅 메시지 로그를 기록 thread
 */
class LogThread : public QThread
{
    Q_OBJECT

public:
    explicit LogThread(QObject *parent = nullptr);
    ~LogThread() = default;
    LogThread(const LogThread&) = delete;
    LogThread& operator=(const LogThread&) = delete;

private:
    void run() override; // 1분마다 채팅 로그를 저장

public slots:
    void appendData(QTreeWidgetItem*); // 새 채팅 기록 추가
    void saveData() const;                   // 채팅 로그 저장

private:
    std::vector<QTreeWidgetItem*> m_itemList; // 채팅 로그 tree widget
    std::string m_filename;                 // 로그를 저장하는 파일의 이름
};

#endif // LOGTHREAD_H
