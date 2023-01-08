#ifndef CHATWINDOWFORADMIN_H
#define CHATWINDOWFORADMIN_H

#include <QWidget>
#include <string>

namespace Ui {
class ChatWindowForAdmin;
}

class ChatWindowForAdmin : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindowForAdmin(std::string = "0", std::string = "", \
                                std::string = "", QWidget *parent = nullptr);
    ~ChatWindowForAdmin();
    ChatWindowForAdmin(const ChatWindowForAdmin&) = delete;
    ChatWindowForAdmin& operator=(const ChatWindowForAdmin&) = delete;

    void receiveMessage(const std::string) const;           // 고객으로부터 온 메시지를 표시
    void updateInfo(const std::string, const std::string);  // 고객의 상태 업데이트

private:
    void changeButtonAndEditState(const std::string); // 고객의 상태에 따라 버튼과 입력 창을 변경
    void loadChatLog() const;                     // 저장된 로그파일로부터 이전 채팅 내용을 불러옴

signals:
    void sendMessage(std::string, std::string); // 고객에게 메시지를 보내도록 하는 시그널
    void inviteClient(std::string);         // 고객을 초대하도록 하는 시그널
    void kickOutClient(std::string);        // 고객을 강퇴하도록 하는 시그널

private slots:
    void on_inputLineEdit_returnPressed(); // 입력 창에서 엔터키를 눌렀을 때의 슬롯
    void on_sendPushButton_clicked();      // send 버튼을 눌렀을 때의 슬롯
    void on_connectPushButton_clicked();   // 고객 초대/강퇴 버튼을 눌렀을 때의 슬롯

private:
    Ui::ChatWindowForAdmin *m_ui;
    const std::string m_clientId;       // 고객 ID
    std::string m_clientName;     // 고객 이름
    std::string m_clientState;    // 고객 상태
};

#endif // CHATWINDOWFORADMIN_H
