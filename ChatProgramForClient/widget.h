#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDataStream>
#include <QAbstractSocket>
#include <string>

class QTcpSocket;
class QFile;
class QProgressDialog;
class LogThread;

typedef enum {
    Chat_Login,    // 로그인(connect)
    Chat_In,       // 채팅 참여
    Chat_Talk,     // 채팅 주고 받기
    Chat_Out,      // 채팅 나가기
    Chat_LogOut,   // 로그 아웃(disconnect)
    Chat_Invite,   // 초대
    Chat_KickOut,  // 강퇴
} Chat_Status;

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

/**
 * @brief 고객을 위한 채팅 프로그램
 */
class Widget : public QWidget
{
    Q_OBJECT

public:
    const int PORT_NUMBER;

    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void receiveData();	   // 관리자(서버)로부터 메시지를 받기 위한 슬롯
    void sendData() const;       // 관리자(서버)로 메시지를 보내기 위한 슬롯
    void disconnect();     // 서버와의 연결이 끊어졌을 때의 슬롯
    // 프로토콜에 따라 서버로 데이터를 전송하기 위한 슬롯
    void sendProtocol(const Chat_Status, const char*, const int = 1020) const;
    void sendFile();       // 관리자(서버)로 파일을 보내기 위한 슬롯
    void goOnSend(const qint64); // 파일을 여러 번 나눠서 전송하기 위한 슬롯

private:
    Ui::Widget *ui;

    void closeEvent(QCloseEvent*) override;
    void loadData(int, std::string) const;     // 이전 채팅 내용 불러오기

    QTcpSocket *clientSocket;		 // 채팅을 위한 소켓
    QTcpSocket *fileClient;          // 파일 전송을 위한 소켓
    QProgressDialog* progressDialog; // 파일 전송 진행 상태
    QFile* file;                     // 관리자(서버)로 보내는 파일
    long long loadSize;                 // 데이터 전송 단위 크기
    long long byteToWrite;              // 전송할 남은 데이터의 크기
    long long totalSize;                // 총 데이터의 크기(파일 + 파일 정보)
    QByteArray outBlock;             // 파일 전송을 위한 블록
    bool isSent;                     // 파일 서버와의 연결 flag

    LogThread* logThread; // 채팅 로그를 저장하기 위한 thread
};
#endif // WIDGET_H
