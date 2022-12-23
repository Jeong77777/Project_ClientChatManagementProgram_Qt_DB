#include "chatwindowforadmin.h"
#include "ui_chatwindowforadmin.h"

#include <QDir>

/**
* @brief 생성자, 버튼,입력창 설정, 창 이름 설정, 이전 채팅 내용 불러오기
*/
ChatWindowForAdmin::ChatWindowForAdmin(std::string id, std::string name, \
                                       std::string state, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWindowForAdmin), \
    clientId(id), clientName(name), clientState(state)
{
    ui->setupUi(this);

    // 버튼 초기화, 입력창 초기화
    changeButtonAndEditState(state);

    // 창 이름 설정
    setWindowTitle(QString::fromStdString\
                   (clientId + " " + clientName + " | " + clientState));

    // 이전 채팅 내용 불러오기
    loadChatLog();
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
* @Param std::string strData 고객으로부터 받은 메시지
*/
void ChatWindowForAdmin::receiveMessage(std::string strData)
{
    ui->messageTextEdit->append(QString::fromStdString\
                                ("<font color=blue>" + clientName + "</font> : " + strData));
}

/**
* @brief 고객의 상태 업데이트
* @Param std::string name 변경할 이름
* @Param std::string state 변경할 상태
*/
void ChatWindowForAdmin::updateInfo(std::string name, std::string state)
{
    // 입력 값이 있을 때만 변경
    if(name.length())
        clientName = name;
    if(state.length())
        clientState = state;

    // 고객의 상태 변경에 따라서 버튼과 입력 창의 상태도 변경
    changeButtonAndEditState(state);

    // 창 이름 변경
    setWindowTitle(QString::fromStdString\
                   (clientId + " " + clientName + " | " + clientState));
}

/**
* @brief 입력 창에서 엔터키를 눌렀을 때의 슬롯
*/
void ChatWindowForAdmin::on_inputLineEdit_returnPressed()
{
    /* 입력 창에 입력된 메시지가 채팅 서버를 통해서 전송되도록 한다. */
    std::string str = ui->inputLineEdit->text().toStdString(); // 입력된 메시지
    if(str.length()) {
        ui->messageTextEdit->append("<font color=red>" + \
                                    tr("Admin") + "</font> : " + QString::fromStdString(str));
        // 고객에게 메시지를 보내도록 하는 시그널 emit
        emit sendMessage(clientId, ui->inputLineEdit->text().toStdString());
        ui->inputLineEdit->clear();
    }
}

/**
* @brief send 버튼을 눌렀을 때의 슬롯
*/
void ChatWindowForAdmin::on_sendPushButton_clicked()
{
    /* 입력 창에 입력된 메시지가 채팅 서버를 통해서 전송되도록 한다. */
    std::string str = ui->inputLineEdit->text().toStdString(); // 입력된 메시지
    if(str.length()) {
        ui->messageTextEdit->append("<font color=red>" + \
                                    tr("Admin") + "</font> : " + QString::fromStdString(str));
        // 고객에게 메시지를 보내도록 하는 시그널 emit
        emit sendMessage(clientId, ui->inputLineEdit->text().toStdString());
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
* @Param std::string state 고객의 상태
*/
void ChatWindowForAdmin::changeButtonAndEditState(std::string state)
{
    if(state == tr("Offline").toStdString()) {
        ui->sendPushButton->setDisabled(true);
        ui->inputLineEdit->setDisabled(true);
        ui->connectPushButton->setDisabled(true);
        ui->connectPushButton->setText(tr("Invite"));
    }
    else if(state == tr("Online").toStdString()) {
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

/**
* @brief 저장된 로그파일로부터 이전 채팅 내용을 불러옴
*/
void ChatWindowForAdmin::loadChatLog()
{
    // 로그 파일 검색
    std::string sPath = QDir::currentPath().toStdString();
    std::string sExt = ".txt";
    std::string sWildcard = "*";
    QStringList lFindList;
    lFindList << QString::fromStdString("log_20" + sWildcard + sExt);  // "log_20"으로 시작하는 txt 파일

    QDir dir(QString::fromStdString(sPath));
    dir.setNameFilters(lFindList);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList lFileInfoList;
    lFileInfoList = dir.entryInfoList();

    // 검색된 로그 파일들로부터 이전 채팅 내용 불러오기
    for (int i = 0 ; i < lFileInfoList.count() ; i++) {
        qDebug() <<lFileInfoList.at(i).absoluteFilePath();

        QFile file(lFileInfoList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QList<QString> row = line.split(" | ");
            if(row.size()) {
                // 고객이 전송한 채팅
                if(row[1].left(5) == QString::fromStdString(clientId)
                        && row[3].contains("(8000)")) {
                    QString data = "<font color=blue>" \
                            + QString::fromStdString(clientName) + "</font> : " + row[2];
                    ui->messageTextEdit->append(data);
                }
                // 관리자가 전송한 채팅
                else if(row[1].left(5) == "10000" \
                        && row[4].left(5) == QString::fromStdString(clientId)) {
                    QString data = "<font color=red>" \
                            + tr("Admin") + "</font> : " + row[2];
                    ui->messageTextEdit->append(data);
                }
            }
        }
        file.close( );
    }
}
