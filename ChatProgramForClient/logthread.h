#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <QStringList>

/**
 * @brief 채팅 메시지 로그를 기록 thread
 */
class LogThread : public QThread
{
    Q_OBJECT
public:
    explicit LogThread(int id = 0, QString name = "", QObject *parent = nullptr);

private:
    void run(); // 1분마다 채팅 로그를 저장

    QStringList chatList;             // 채팅 로그 tree widget
    QString filename;                 // 로그를 저장하는 파일의 이름

public slots:
    void appendData(QString);          // 새 채팅 기록 추가
    void saveData();                   // 채팅 로그 저장
};

#endif // LOGTHREAD_H
