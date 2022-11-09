#include "chatwindowforadmin.h"
#include "ui_chatwindowforadmin.h"

/**
* @brief 생성자, 버튼,입력창 설정, 창 이름 설정
*/
ChatWindowForAdmin::ChatWindowForAdmin(QString id, QString name, QString state, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWindowForAdmin),  clientId(id), clientName(name), clientState(state)
{
    ui->setupUi(this);

    changeButtonAndEditState(state);

    setWindowTitle(clientId + " " + clientName + " | " + clientState);
}

/**
* @brief 소멸자
*/
ChatWindowForAdmin::~ChatWindowForAdmin()
{
    delete ui;
}

/**
* @brief 고객으로부터 온 메시지를 표시
* @Param QString strData 고객으로부터 받은 메시지
*/
void ChatWindowForAdmin::receiveMessage(QString strData)
{
    ui->messageTextEdit->append("<font color=blue>" + \
                                clientName + "</font> : " + strData);
}

/**
* @brief 고객의 상태 업데이트
* @Param QString name 변경할 이름
* @Param QString state 변경할 상태
*/
void ChatWindowForAdmin::updateInfo(QString name, QString state)
{
    // 입력 값이 있을 때만 변경
    if(name.length())
        clientName = name;
    if(state.length())
        clientState = state;

    // 고객의 상태 변경에 따라서 버튼과 입력 창도 변경
    changeButtonAndEditState(state);

    // 창 이름 변경
    setWindowTitle(clientId + " " + clientName + " | " + clientState);
}

/**
* @brief 입력 창에서 엔터키를 눌렀을 때의 슬롯
*/
void ChatWindowForAdmin::on_inputLineEdit_returnPressed()
{
    QString str = ui->inputLineEdit->text();
    if(str.length()) {
        ui->messageTextEdit->append("<font color=red>" + \
                                    tr("Admin") + "</font> : " + str);
        emit sendMessage(clientId, ui->inputLineEdit->text());
        ui->inputLineEdit->clear();
    }
}

/**
* @brief send 버튼을 눌렀을 때의 슬롯
*/
void ChatWindowForAdmin::on_sendPushButton_clicked()
{
    /* 고객에게 메시지를 보내도록 하는 시그널 emit */
    QString str = ui->inputLineEdit->text();
    if(str.length()) {
        ui->messageTextEdit->append("<font color=red>" + \
                                    tr("Admin") + "</font> : " + str);
        emit sendMessage(clientId, ui->inputLineEdit->text());
        ui->inputLineEdit->clear();
    }
}

/**
* @brief 고객 초대/강퇴 버튼을 눌렀을 때의 슬롯
*/
void ChatWindowForAdmin::on_connectPushButton_clicked()
{
    /* 고객을 초대/강퇴 하도록 하는 시그널 emit */
    if(ui->connectPushButton->text() == tr("Invite")) // 초대
        emit inviteClient(clientId);
    else                                              // 강퇴
        emit kickOutClient(clientId);
}

/**
* @brief 고객의 상태에 따라 버튼과 입력 창을 변경
* @Param 고객의 상태
*/
void ChatWindowForAdmin::changeButtonAndEditState(QString state)
{
    if(state == tr("Offline")) {
        ui->sendPushButton->setDisabled(true);
        ui->inputLineEdit->setDisabled(true);
        ui->connectPushButton->setDisabled(true);
        ui->connectPushButton->setText(tr("Invite"));
    }
    else if(state == tr("Online")) {
        ui->sendPushButton->setDisabled(true);
        ui->inputLineEdit->setDisabled(true);
        ui->connectPushButton->setEnabled(true);
        ui->connectPushButton->setText(tr("Invite"));
    }
    else { // Chat in
        ui->sendPushButton->setEnabled(true);
        ui->inputLineEdit->setEnabled(true);
        ui->connectPushButton->setEnabled(true);
        ui->connectPushButton->setText(tr("Kick out"));
    }
}
