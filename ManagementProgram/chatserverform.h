#ifndef CHATSERVERFORM_H
#define CHATSERVERFORM_H

#include <QWidget>
#include <QHash>
#include <string>

class QMenu;
class QTcpServer;
class QTcpSocket;
class QFile;
class QProgressDialog;
class LogThread;
class ChatWindowForAdmin;

namespace Ui {
class ChatServerForm;
}

typedef enum {
    Chat_Login,    // 로그인(connect)
    Chat_In,       // 채팅 참여
    Chat_Talk,     // 채팅 주고 받기
    Chat_Out,      // 채팅 나가기
    Chat_LogOut,   // 로그 아웃(disconnect)
    Chat_Invite,   // 초대
    Chat_KickOut,  // 강퇴
} Chat_Status;


/**
* @brief 채팅 서버를 관리하는 클래스
*/
class ChatServerForm : public QWidget
{
    Q_OBJECT

public:
    explicit ChatServerForm(QWidget *parent = nullptr);
    ~ChatServerForm();

private slots:
    // 고객 정보 관리 객체로부터 받은 고객 정보를 리스트에 추가(변경)하는 슬롯
    void addClient(int, std::string);

    /* 파일 서버 */
    void acceptConnection(); // 새로운 연결을 위한 슬롯
    void readClient();       // 파일을 수신하는 슬롯

    /* 채팅 서버 */
    void clientConnect( ); // 새로운 연결을 위한 슬롯
    void receiveData( );   // 메시지를 받는 슬롯
    void removeClient( );  // 고객과의 연결을 끊겼을 때의 슬롯
    void openChatWindow(); // 관리자의 채팅창을 여는 슬롯
    void inviteClient();   // 고객을 채팅방에 초대하는 슬롯
    // 관리자의 채팅창에서 고객을 채팅방에 초대하는 슬롯
    void inviteClientInChatWindow(std::string);
    void kickOut();        // 고객을 채팅방에서 강퇴하는 슬롯
    // 관리자의 채팅창에서 고객을 채팅방으로부터 강퇴하는 슬롯
    void kickOutInChatWindow(QString);
    // 관리자가 고객에게 채팅을 전송하기 위한 슬롯
    void sendData(QString, QString);

    // 고객 리스트 tree widget의 context menu 슬롯
    void on_clientTreeWidget_customContextMenuRequested(const QPoint &pos);

private:
    const int BLOCK_SIZE;  // 블록 사이즈
    const int PORT_NUMBER; // 채팅을 위한 port number

    // 고객에게 로그인 결과를 전송
    void sendLoginResult(QTcpSocket*, const char*);

    Ui::ChatServerForm *ui; // ui
    QTcpServer *chatServer; // 채팅을 위한 서버
    QTcpServer *fileServer; // 파일 수신을 위한 서버

    // <port, id>를 저장하는 hash
    QHash<quint16, QString> portClientIdHash;
    // <id, socket>을 저장하는 hash
    QHash<QString, QTcpSocket*> clientIdSocketHash;
    // <id, name>을 저장하는 hash
    QHash<QString, QString> clientIdNameHash;
    // <id, chat window>을 저장하는 hash
    QHash<QString, ChatWindowForAdmin*> clientIdWindowHash;

    QMenu* menu; // 고객 리스트 tree widget context 메뉴

    QFile* file;                     // 수신 받는 파일
    QProgressDialog* progressDialog; // 파일 수신 진행 상태
    qint64 totalSize;                // 총 데이터의 크기(파일 + 파일 정보)
    qint64 byteReceived;             // 수신한 누적 데이터의 크기
    QByteArray inBlock;              // 파일 수신을 위한 블록

    LogThread* logThread; // 채팅 로그를 저장하기 위한 thread
};

#endif // CHATSERVERFORM_H
