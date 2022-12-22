#include "chatserverform.h"
#include "ui_chatserverform.h"
#include "logthread.h"
#include "chatwindowforadmin.h"

#include <QList>
#include <QPushButton>
#include <QBoxLayout>
#include <QTcpServer>
#include <QTcpSocket>
#include <QApplication>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QDebug>
#include <QMenu>
#include <QFile>
#include <QFileInfo>
#include <QProgressDialog>
#include <cassert>


/**
* @brief 생성자, split 사이즈 설정, 채팅,파일 서버 생성, context 메뉴 설정
* @brief log thread 생성
*/
ChatServerForm::ChatServerForm(QWidget *parent) :
    QWidget(parent), BLOCK_SIZE(1024), PORT_NUMBER(8000),
    ui(new Ui::ChatServerForm),
    chatServer(nullptr), fileServer(nullptr),
    menu(nullptr), file(nullptr), progressDialog(nullptr),
    totalSize(0), byteReceived(0), inBlock(0), logThread(nullptr)
{
    ui->setupUi(this);

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 150 << 470;
    ui->splitter->setSizes(sizes);

    /* 고객 리스트 tree widget의 열 너비를 설정 */
    ui->clientTreeWidget->QTreeView::setColumnWidth(0,90);
    ui->clientTreeWidget->QTreeView::setColumnWidth(1,50);
    ui->clientTreeWidget->QTreeView::setColumnWidth(2,50);

    /* 채팅 서버 생성 */
    chatServer = new QTcpServer(this); // tcpserver를 만들어줌
    assert(connect(chatServer, SIGNAL(newConnection()), SLOT(clientConnect())));
    if (!chatServer->listen(QHostAddress::Any, PORT_NUMBER)) {
        QMessageBox::critical(this, tr("Chatting Server"), \
                              tr("Unable to start the server: %1.") \
                              .arg(chatServer->errorString()));
        close( );
        return;
    }

    /* 파일 서버 생성 */
    fileServer = new QTcpServer(this);
    assert(connect(fileServer, SIGNAL(newConnection()), SLOT(acceptConnection())));
    if (!fileServer->listen(QHostAddress::Any, PORT_NUMBER+1)) {
        QMessageBox::critical(this, tr("Chatting Server"), \
                              tr("Unable to start the server: %1.") \
                              .arg(fileServer->errorString( )));
        close( );
        return;
    }

    qDebug("Start listening ...");

    /* 고객 리스트 tree widget의 context 메뉴 설정 */
    QAction* openAction = new QAction(tr("Open chat window"));
    openAction->setObjectName("Open");
    assert(connect(openAction, SIGNAL(triggered()), SLOT(openChatWindow())));

    QAction* inviteAction = new QAction(tr("Invite"));
    inviteAction->setObjectName("Invite");
    assert(connect(inviteAction, SIGNAL(triggered()), SLOT(inviteClient())));

    QAction* kickOutAction = new QAction(tr("Kick out"));
    assert(connect(kickOutAction, SIGNAL(triggered()), SLOT(kickOut())));

    menu = new QMenu;
    menu->addAction(openAction);
    menu->addAction(inviteAction);
    menu->addAction(kickOutAction);
    ui->clientTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    /* 파일 수신 진행 상태를 보여주는 progress dialog 설정 */
    progressDialog = new QProgressDialog(0);
    progressDialog->setAutoClose(true);
    progressDialog->reset();

    /* 채팅 로그를 저장하기 위한 thread 생성 */
    logThread = new LogThread(this);
    logThread->start();
    // save 버튼을 누르면 logThread에서 로그가 저장되도록 connect
    assert(connect(ui->savePushButton, SIGNAL(clicked()), logThread, SLOT(saveData())));

    qDebug() << tr("The server is running on port %1.").arg(chatServer->serverPort( ));
}

/**
* @brief 소멸자, log thread 종료, 채팅,파일 서버 닫기
*/
ChatServerForm::~ChatServerForm()
{
    delete ui;

    logThread->saveData();
    logThread->terminate();
    chatServer->close( );
    fileServer->close( );
}

/**
* @brief 고객 정보 관리 객체로부터 받은 고객 정보를 리스트에 추가(변경)하는 슬롯
* @Param int indId 고객 ID
* @Param std::string name 고객 이름
*/
void ChatServerForm::addClient(int intId, std::string name)
{
    QString id = QString::number(intId); // int->QString 변환

    /* 리스트에 이미 등록된 고객이면 정보를 변경 */
    foreach(auto c, ui->clientTreeWidget->findItems(id, Qt::MatchFixedString, 1)) {
        c->setText(2, QString::fromStdString(name));
        clientIdNameHash[id] = QString::fromStdString(name);
        if(clientIdWindowHash.contains(id))
            clientIdWindowHash[id]->updateInfo(QString::fromStdString(name), "");
        return;
    }

    /* 리스트에 등록되지 않은 고객이면 새로 추가 */
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->clientTreeWidget);
    item->setText(0, tr("Offline"));
    item->setIcon(0, QIcon(":/images/Red-Circle.png"));
    item->setText(1, id);
    item->setText(2, QString::fromStdString(name));
    ui->clientTreeWidget->addTopLevelItem(item);
    clientIdNameHash[id] = QString::fromStdString(name);
}

/**
* @brief 새로운 연결이 들어오면 파일 수신을 위한 소켓을 생성하는 슬롯
*/
void ChatServerForm::acceptConnection()
{
    qDebug("Connected, preparing to receive files!");

    QTcpSocket* receivedSocket = fileServer->nextPendingConnection();
    // 파일을 수신하면 읽어들일 수 있도록 connect
    assert(connect(receivedSocket, SIGNAL(readyRead()), this, SLOT(readClient())));
}

/**
* @brief 파일을 수신하는 슬롯
*/
void ChatServerForm::readClient()
{
    qDebug("Receiving file ...");
    QTcpSocket* receivedSocket = dynamic_cast<QTcpSocket *>(sender( ));
    QString filename, id;

    /* 파일 수신 시작 */
    if (byteReceived == 0) {
        progressDialog->reset(); // 파일 수신 진행 상태를 나타내는 progress dialog 초기화
        progressDialog->show();  // progress dialog 보여주기

        QString ip = receivedSocket->peerAddress().toString();
        quint16 port = receivedSocket->peerPort();
        qDebug() << ip << " : " << port;

        QDataStream in(receivedSocket);
        in >> totalSize >> byteReceived >> filename >> id;
        // progressDialog의 최대값을 총 데이터의 크기(파일 + 파일 정보)로 설정
        progressDialog->setMaximum(totalSize);

        /* 로그 tree widget에 채팅 로그 기록 */
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->messageTreeWidget);
        // Sender IP(Port)
        item->setText(0, ip+"("+QString::number(port)+")");
        // Sende ID(Name)
        item->setText(1, id+"("+clientIdNameHash[id]+")");
        // filename
        item->setText(2, filename);
        // Receiver IP(Port)
        item->setText(3, fileServer->serverAddress().toString()+ \
                      "("+QString::number(PORT_NUMBER+1)+")" );
        // Receiver ID(name) = 10000(Admin)
        item->setText(4, QString("10000")+"("+tr("Admin")+")");
        // Time
        item->setText(5, QDateTime::currentDateTime().toString());
        item->setToolTip(2, filename);

        // 컨텐츠의 길이로 채팅 로그 tree widget의 헤더의 크기를 조정
        for(int i = 0; i < ui->messageTreeWidget->columnCount(); i++)
            ui->messageTreeWidget->resizeColumnToContents(i);

        ui->messageTreeWidget->addTopLevelItem(item);
        logThread->appendData(item);

        QFileInfo info(filename);                  // 파일의 정보를 가져옴
        QString currentFileName = info.fileName(); // 파일의 경로에서 이름만 뽑아옴
        file = new QFile(currentFileName);         // 파일 생성
        file->open(QFile::WriteOnly|QFile::Truncate);
    }
    /* 파일 수신 중 */
    else {
        // 파일 데이터를 읽어서 저장
        inBlock = receivedSocket->readAll();
        byteReceived += inBlock.size(); // 수신한 누적 데이터의 크기
        file->write(inBlock);
        file->flush();
    }

    // progress dialog를 수신한 누적 데이터의 크기로 설정
    progressDialog->setValue(byteReceived);

    /* 파일을 다 수신하면 QFile 객체를 닫고 삭제 */
    if (byteReceived == totalSize) {
        qDebug() << QString("%1 receive completed").arg(filename);

        // 파일 수신 관련 변수 초기화
        inBlock.clear();
        byteReceived = 0;
        totalSize = 0;

        // progress dialog 닫기
        progressDialog->reset();
        progressDialog->hide();

        // file 객체 닫고 삭제
        file->close();
        delete file;
    }
}

/**
* @brief 새로운 연결이 들어오면 채팅을 위한 소켓을 생성하는 슬롯
*/
void ChatServerForm::clientConnect( )
{
    QTcpSocket *clientConnection = chatServer->nextPendingConnection();
    // 메시지가 오면 읽어들일 수 있도록 connect
    assert(connect(clientConnection, SIGNAL(readyRead()), SLOT(receiveData())));
    // 연결이 끊어지면 고객의 상태를 offline으로 변경하도록 connect
    assert(connect(clientConnection, SIGNAL(disconnected()), SLOT(removeClient())));
    qDebug("new connection is established...");
}

/**
* @brief 메시지를 받는 슬롯
*/
void ChatServerForm::receiveData( )
{
    /* 메시지 수신 */
    QTcpSocket *clientConnection = dynamic_cast<QTcpSocket *>(sender( ));
    QByteArray bytearray = clientConnection->read(BLOCK_SIZE);

    Chat_Status type;       // 채팅의 목적
    char data[1020];        // 전송되는 메시지/데이터
    memset(data, 0, 1020);

    QDataStream in(&bytearray, QIODevice::ReadOnly);
    in.device()->seek(0);
    in >> type;
    in.readRawData(data, 1020);

    QString ip = clientConnection->peerAddress().toString();
    quint16 port = clientConnection->peerPort();
    QString strData = QString::fromStdString(data);

    qDebug() << ip << " : " << type;


    switch(type) {
    case Chat_Login: { // 로그인
        QList<QString> row = strData.split(", "); // row[0] = id, row[1] = name
        QString id = row[0];
        QString name = row[1];
        /* 고객 리스트에서 ID와 이름을 확인 */
        foreach(auto item, ui->clientTreeWidget-> \
                findItems(id, Qt::MatchFixedString, 1)) {

            // 고객 리스트에 ID와 이름이 있으면 로그인 허가
            if(item->text(2) == name) {
                // 고객 리스트에서 고객의 상태를 online으로 변경
                if(item->text(0) != tr("Online")) {
                    item->setText(0, tr("Online"));
                    item->setIcon(0, QIcon(":/images/Blue-Circle.png"));

                    // clientIdSocketHash에 <id, socket> 추가
                    clientIdSocketHash[id] = clientConnection;
                    portClientIdHash[port] = id;

                    // 로그인을 허가한다는 메시지 전송
                    sendLoginResult(clientConnection, "permit");

                    // 관리자의 채팅창에서 고객의 상태를 online 변경
                    if(clientIdWindowHash.contains(item->text(1)))
                        // 채팅창이 이미 만들어져 있으면 고객 상태 변경
                        clientIdWindowHash[item->text(1)]->updateInfo("", tr("Online"));
                    else { // 채팅창이 만들어져 있지 않으면 새로 만들고 설정
                        ChatWindowForAdmin* w = new ChatWindowForAdmin(id, name, tr("Online"));
                        clientIdWindowHash[id] = w;
                        assert(connect(w, SIGNAL(sendMessage(std::string,std::string)), this, SLOT(sendData(std::string,std::string))));
                        assert(connect(w, SIGNAL(inviteClient(std::string)), this, SLOT(inviteClientInChatWindow(std::string))));
                        assert(connect(w, SIGNAL(kickOutClient(std::string)), this, SLOT(kickOutInChatWindow(std::string))));
                    }
                    return;
                }

            }
        }
        // 고객 리스트에 없는 ID와 이름이면 로그인 거부
        sendLoginResult(clientConnection, "forbid"); // 로그인을 거부하는 메시지 전송
        clientConnection->disconnectFromHost();      // 연결 끊기
    }
        break;


    case Chat_In: // 고객이 채팅에 참여
        foreach(auto item, ui->clientTreeWidget->findItems(strData, Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경 */
            if(item->text(0) != tr("Chat in")) {
                item->setText(0, tr("Chat in"));
                item->setIcon(0, QIcon(":/images/Green-Circle.png"));

                if(clientIdWindowHash.contains(item->text(1)))
                    clientIdWindowHash[item->text(1)]->updateInfo("", tr("Chat in"));
            }
            if(clientIdSocketHash.contains(strData))
                clientIdSocketHash[strData] = clientConnection;
        }
        break;


    case Chat_Talk: { // 채팅 주고 받기
        if(clientIdWindowHash.contains(portClientIdHash[port]))
            clientIdWindowHash[portClientIdHash[port]]->receiveMessage(strData);

        /* 로그 tree widget에 채팅 로그 기록 */
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->messageTreeWidget);
        // Sender IP(Port)
        item->setText(0, ip+"("+QString::number(port)+")");
        // Sende ID(Name)
        item->setText(1, portClientIdHash[port]+ \
                      "("+clientIdNameHash[portClientIdHash[port]]+")");
        // message
        item->setText(2, QString(data));
        // Receiver IP(Port)
        item->setText(3, chatServer->serverAddress().toString()+ \
                      "("+QString::number(PORT_NUMBER)+")" );
        // Receiver ID(name) = 10000(Admin)
        item->setText(4, QString("10000")+"("+tr("Admin")+")");
        // Time
        item->setText(5, QDateTime::currentDateTime().toString());
        item->setToolTip(2, QString(data));
        ui->messageTreeWidget->addTopLevelItem(item);

        for(int i = 0; i < ui->messageTreeWidget->columnCount(); i++)
            ui->messageTreeWidget->resizeColumnToContents(i);

        logThread->appendData(item);
    }
        break;


    case Chat_Out: // 고객이 채팅에서 나감
        foreach(auto item, ui->clientTreeWidget->findItems(strData, Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경 */
            if(item->text(0) != tr("Online")) {
                item->setText(0, tr("Online"));
                item->setIcon(0, QIcon(":/images/Blue-Circle.png"));

                if(clientIdWindowHash.contains(item->text(1)))
                    clientIdWindowHash[item->text(1)]->updateInfo("", tr("Online"));
            }
        }
        break;


    case Chat_LogOut: // 고객이 log out
        foreach(auto item, ui->clientTreeWidget->findItems(strData, Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 offline으로 변경 */
            if(item->text(0) != tr("Offline")) {
                item->setText(0, tr("Offline"));
                item->setIcon(0, QIcon(":/images/Red-Circle.png"));

                if(clientIdWindowHash.contains(item->text(1)))
                    clientIdWindowHash[item->text(1)]->updateInfo("", tr("Offline"));
            }
            clientIdSocketHash.remove(portClientIdHash[port]);
            portClientIdHash.remove(port);
            clientConnection->disconnectFromHost();
        }
        break;


    default:
        break;
    }
}

/**
* @brief 고객과의 연결이 끊어졌을 때의 슬롯, 고객의 상태를 offline으로 변경
*/
void ChatServerForm::removeClient()
{
    QTcpSocket *clientConnection = dynamic_cast<QTcpSocket *>(sender( ));
    if(clientConnection != nullptr) {

        // 고객 리스트와 관리자 채팅창에서 고객의 상태를 offline으로 변경
        QString id = portClientIdHash[clientConnection->peerPort()];
        foreach(auto item, ui->clientTreeWidget->findItems(id, Qt::MatchFixedString, 1)) {
            qDebug() << item->text(2);
            item->setText(0, tr("Offline"));
            item->setIcon(0, QIcon(":/images/Red-Circle.png"));
            if(clientIdWindowHash.contains(id))
                clientIdWindowHash[id]->updateInfo("", tr("Offline"));
        }

        /* 소켓 삭제 */
        clientIdSocketHash.remove(id);
        clientConnection->deleteLater();
    }
}

/**
* @brief 관리자의 채팅창을 여는 슬롯
*/
void ChatServerForm::openChatWindow()
{
    QString id = ui->clientTreeWidget->currentItem()->text(1);
    QString state;


    if(false == clientIdWindowHash.contains(id)) { // 채팅창이 만들어져 있지 않으면 새로 만듦
        foreach(auto item, ui->clientTreeWidget->findItems(id, Qt::MatchFixedString, 1)) {
            state = item->text(0);
        }
        ChatWindowForAdmin* w = new ChatWindowForAdmin(id, clientIdNameHash[id], state);
        clientIdWindowHash[id] = w;
        w->show();
        assert(connect(w, SIGNAL(sendMessage(std::string,std::string)), this, SLOT(sendData(std::string,std::string))));
        assert(connect(w, SIGNAL(inviteClient(std::string)), this, SLOT(inviteClientInChatWindow(std::string))));
        assert(connect(w, SIGNAL(kickOutClient(std::string)), this, SLOT(kickOutInChatWindow(std::string))));
    }
    else {                                         // 채팅창이 이미 만들어져 있으면 열기만 함
        clientIdWindowHash[id]->showNormal();
        clientIdWindowHash[id]->activateWindow();
    }
}

/**
* @brief 고객을 채팅방에 초대하는 슬롯
*/
void ChatServerForm::inviteClient()
{
    if(ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객을 채팅방에 초대 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Invite;
    out.writeRawData("", 1020);

    // 현재 선택된 item에 표시된 ID와 해쉬로 부터 소켓을 가져옴
    QString id = ui->clientTreeWidget->currentItem()->text(1);
    QTcpSocket* sock = clientIdSocketHash[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경
    ui->clientTreeWidget->currentItem()->setText(0, tr("Chat in"));
    ui->clientTreeWidget->currentItem()->setIcon(0, QIcon(":/images/Green-Circle.png"));
    if(clientIdWindowHash.contains(id))
        clientIdWindowHash[id]->updateInfo("", tr("Chat in"));
}

/**
* @brief 관리자의 채팅창에서 고객을 채팅방에 초대하는 슬롯
* @Param std::string id 초대할 고객의 id
*/
void ChatServerForm::inviteClientInChatWindow(std::string id)
{
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Invite;
    out.writeRawData("", 1020);

    // ID와 해쉬로 부터 소켓을 가져옴
    QTcpSocket* sock = clientIdSocketHash[QString::fromStdString(id)];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경
    foreach(auto item, ui->clientTreeWidget-> \
            findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
        item->setText(0, tr("Chat in"));
        item->setIcon(0, QIcon(":/images/Green-Circle.png"));
    }
    if(clientIdWindowHash.contains(QString::fromStdString(id)))
        clientIdWindowHash[QString::fromStdString(id)]->updateInfo("", tr("Chat in"));
}

/**
* @brief 고객을 채팅방에서 강퇴하는 슬롯
*/
void ChatServerForm::kickOut()
{
    if(ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객을 채팅방에서 강퇴 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_KickOut;
    out.writeRawData("", 1020);

    // 현재 선택된 item에 표시된 ID와 해쉬로 부터 소켓을 가져옴
    QString id = ui->clientTreeWidget->currentItem()->text(1);
    QTcpSocket* sock = clientIdSocketHash[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경
    ui->clientTreeWidget->currentItem()->setText(0, tr("Online"));
    ui->clientTreeWidget->currentItem()->setIcon(0, QIcon(":/images/Blue-Circle.png"));
    if(clientIdWindowHash.contains(id))
        clientIdWindowHash[id]->updateInfo("", tr("Online"));
}

/**
* @brief 관리자의 채팅창에서 고객을 채팅방으로부터 강퇴하는 슬롯
* @Param std::string id 강퇴할 고객의 id
*/
void ChatServerForm::kickOutInChatWindow(std::string id)
{
    /* 고객을 채팅방에서 강퇴 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_KickOut;
    out.writeRawData("", 1020);

    // ID와 해쉬로 부터 소켓을 가져옴
    QTcpSocket* sock = clientIdSocketHash[QString::fromStdString(id)];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경
    foreach(auto item, ui->clientTreeWidget-> \
            findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
        item->setText(0, tr("Online"));
        item->setIcon(0, QIcon(":/images/Blue-Circle.png"));
    }
    if(clientIdWindowHash.contains(QString::fromStdString(id)))
        clientIdWindowHash[QString::fromStdString(id)]->updateInfo("", tr("Online"));
}

/**
* @brief 관리자가 고객에게 채팅을 전송하기 위한 슬롯
* @Param std::string 메시지를 받을 고객의 id
* @Param std::string str 전달할 메시지
*/
void ChatServerForm::sendData(std::string id, std::string str)
{

    if(false == clientIdSocketHash.contains(QString::fromStdString(id)))
        return;

    /* 메시지 전송 */
    QTcpSocket* sock = clientIdSocketHash[QString::fromStdString(id)];
    QString data;
    QByteArray sendArray;
    sendArray.clear();
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Talk;
    data = "<font color=blue>" + tr("Admin") + "</font> : " + QString::fromStdString(str);
    out.writeRawData(data.toStdString().data(), 1020);
    sock->write(sendArray);

    /* 로그 tree widget에 채팅 로그 기록 */
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->messageTreeWidget);
    // Sender IP(Port)
    item->setText(0, chatServer->serverAddress().toString()+\
                  "("+QString::number(PORT_NUMBER)+")" );
    // Sender ID(name) = 10000(Admin)
    item->setText(1, QString("10000")+"("+tr("Admin")+")");
    // message
    item->setText(2, QString::fromStdString(str));
    // Receiver IP(Port)
    item->setText(3, sock->peerAddress().toString()+ \
                  "("+QString::number(sock->peerPort())+")");
    // Receiver ID(name)
    item->setText(4, QString::fromStdString(id)+"("+clientIdNameHash[QString::fromStdString(id)]+")");
    // Time
    item->setText(5, QDateTime::currentDateTime().toString());
    item->setToolTip(2, QString::fromStdString(str));
    ui->messageTreeWidget->addTopLevelItem(item);

    for(int i = 0; i < ui->messageTreeWidget->columnCount(); i++)
        ui->messageTreeWidget->resizeColumnToContents(i);

    logThread->appendData(item);
}

/**
* @brief 고객 리스트 tree widget의 context menu 슬롯
* const QPoint &pos 우클릭한 위치
*/
void ChatServerForm::on_clientTreeWidget_customContextMenuRequested(const QPoint &pos)
{
    if(ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객 리스트 tree widget 위에서 우클릭한 위치에서 context menu 출력 */
    foreach(QAction *action, menu->actions()) {
        if(action->objectName() == "Open")        // 관리자 채팅창 열기(항상 활성화)
            action->setEnabled(true);
        else if(action->objectName() == "Invite") // 초대(고객이 online 일때만 활성화)
            action->setEnabled(ui->clientTreeWidget->currentItem()->text(0) == tr("Online"));
        else                                      // 강퇴(고객이 chat in 일때만 활성화)
            action->setEnabled(ui->clientTreeWidget->currentItem()->text(0) == tr("Chat in"));
    }
    QPoint globalPos = ui->clientTreeWidget->mapToGlobal(pos);
    menu->exec(globalPos);
}

/**
* @brief 고객에게 로그인 결과를 전송
* @Param QTcpSocket* sock 고객과 연결된 소켓
* @Param const char* msg 로그인 결과
*/
void ChatServerForm::sendLoginResult(QTcpSocket* sock, const char* msg)
{
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Login;
    out.writeRawData(msg, 1020); // permit or forbid
    sock->write(sendArray);
}
