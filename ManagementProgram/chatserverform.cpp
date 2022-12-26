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
    m_ui(new Ui::ChatServerForm),
    m_chatServer(nullptr), m_fileServer(nullptr),
    m_menu(nullptr), m_file(nullptr), m_progressDialog(nullptr),
    m_totalSize(0), m_byteReceived(0), m_inBlock(0), m_logThread(nullptr),
    m_openAction(nullptr), m_inviteAction(nullptr), m_kickOutAction(nullptr)
{
    m_ui->setupUi(this);

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 150 << 470;
    m_ui->splitter->setSizes(sizes);

    /* 고객 리스트 tree widget의 열 너비를 설정 */
    m_ui->clientTreeWidget->QTreeView::setColumnWidth(0,90);
    m_ui->clientTreeWidget->QTreeView::setColumnWidth(1,50);
    m_ui->clientTreeWidget->QTreeView::setColumnWidth(2,50);

    /* 채팅 서버 생성 */
    m_chatServer = new QTcpServer(this); // tcpserver를 만들어줌
    assert(connect(m_chatServer, SIGNAL(newConnection()), SLOT(clientConnect())));
    if (!m_chatServer->listen(QHostAddress::Any, PORT_NUMBER)) {
        QMessageBox::critical(this, tr("Chatting Server"), \
                              tr("Unable to start the server: %1.") \
                              .arg(m_chatServer->errorString()));
        close( );
        return;
    }

    /* 파일 서버 생성 */
    m_fileServer = new QTcpServer(this);
    assert(connect(m_fileServer, SIGNAL(newConnection()), SLOT(acceptConnection())));
    if (!m_fileServer->listen(QHostAddress::Any, PORT_NUMBER+1)) {
        QMessageBox::critical(this, tr("Chatting Server"), \
                              tr("Unable to start the server: %1.") \
                              .arg(m_fileServer->errorString( )));
        close( );
        return;
    }

    qDebug("Start listening ...");

    /* 고객 리스트 tree widget의 context 메뉴 설정 */
    m_openAction = new QAction(tr("Open chat window"));
    m_openAction->setObjectName("Open");
    assert(connect(m_openAction, SIGNAL(triggered()), SLOT(openChatWindow())));

    m_inviteAction = new QAction(tr("Invite"));
    m_inviteAction->setObjectName("Invite");
    assert(connect(m_inviteAction, SIGNAL(triggered()), SLOT(inviteClient())));

    m_kickOutAction = new QAction(tr("Kick out"));
    assert(connect(m_kickOutAction, SIGNAL(triggered()), SLOT(kickOut())));

    m_menu = new QMenu;
    m_menu->addAction(m_openAction);
    m_menu->addAction(m_inviteAction);
    m_menu->addAction(m_kickOutAction);
    m_ui->clientTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    /* 파일 수신 진행 상태를 보여주는 progress dialog 설정 */
    m_progressDialog = new QProgressDialog(0);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->reset();

    /* 채팅 로그를 저장하기 위한 thread 생성 */
    m_logThread = new LogThread(this);
    m_logThread->start();
    // save 버튼을 누르면 logThread에서 로그가 저장되도록 connect
    assert(connect(m_ui->savePushButton, SIGNAL(clicked()), m_logThread, SLOT(saveData())));

    qDebug() << tr("The server is running on port %1.").arg(m_chatServer->serverPort( ));
}

/**
* @brief 소멸자, log thread 종료, 채팅,파일 서버 닫기
*/
ChatServerForm::~ChatServerForm()
{
    m_logThread->saveData();
    m_logThread->terminate();
    m_chatServer->close( );
    m_fileServer->close( );

    m_chatServer->deleteLater(); m_chatServer = nullptr;
    m_fileServer->deleteLater(); m_fileServer = nullptr;
    delete m_openAction; m_openAction = nullptr;
    delete m_inviteAction; m_inviteAction = nullptr;
    delete m_kickOutAction; m_kickOutAction = nullptr;
    delete m_menu; m_menu = nullptr;
    delete m_progressDialog; m_progressDialog = nullptr;
    m_logThread->deleteLater(); m_logThread = nullptr;

    int row;

    row = m_ui->clientTreeWidget->topLevelItemCount();
    for(int i = 0; i < row; i++) {
        auto item = m_ui->clientTreeWidget->itemAt(0, 0);
        m_ui->clientTreeWidget->takeTopLevelItem(m_ui->clientTreeWidget->indexOfTopLevelItem(item));
        delete item;
    }
    row = m_ui->messageTreeWidget->topLevelItemCount();
    for(int i = 0; i < row; i++) {
        auto item = m_ui->messageTreeWidget->itemAt(0, 0);
        m_ui->messageTreeWidget->takeTopLevelItem(m_ui->messageTreeWidget->indexOfTopLevelItem(item));
        delete item;
    }

    std::unordered_map<std::string, ChatWindowForAdmin*>::iterator iter = m_clientIdWindowMap.begin();
    for(; iter != m_clientIdWindowMap.end(); iter++) {
        delete iter->second; iter->second = nullptr;
    }

    delete m_ui; m_ui = nullptr;

}

/**
* @brief 고객 정보 관리 객체로부터 받은 고객 정보를 리스트에 추가(변경)하는 슬롯
* @Param int indId 고객 ID
* @Param std::string name 고객 이름
*/
void ChatServerForm::addClient(const int intId, const std::string name)
{
    std::string id = std::to_string(intId); // int->std::string 변환

    /* 리스트에 이미 등록된 고객이면 정보를 변경 */
    for(const auto& c : m_ui->clientTreeWidget->findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
        c->setText(2, QString::fromStdString(name));
        m_clientIdNameMap[id] = name;
        if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
            m_clientIdWindowMap[id]->updateInfo(name, "");
        return;
    }

    /* 리스트에 등록되지 않은 고객이면 새로 추가 */
    QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->clientTreeWidget);
    item->setText(0, tr("Offline"));
    item->setIcon(0, QIcon(":/images/Red-Circle.png"));
    item->setText(1, QString::fromStdString(id));
    item->setText(2, QString::fromStdString(name));
    m_ui->clientTreeWidget->addTopLevelItem(item);
    m_clientIdNameMap[id] = name;
}

/**
* @brief 새로운 연결이 들어오면 파일 수신을 위한 소켓을 생성하는 슬롯
*/
void ChatServerForm::acceptConnection() const
{
    qDebug("Connected, preparing to receive files!");

    QTcpSocket* receivedSocket = m_fileServer->nextPendingConnection();
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
    if (m_byteReceived == 0) {
        m_progressDialog->reset(); // 파일 수신 진행 상태를 나타내는 progress dialog 초기화
        m_progressDialog->show();  // progress dialog 보여주기

        std::string ip = receivedSocket->peerAddress().toString().toStdString();
        unsigned short port = receivedSocket->peerPort();
        qDebug() << QString::fromStdString(ip) << " : " << port;

        QDataStream in(receivedSocket);
        in >> m_totalSize >> m_byteReceived >> filename >> id;
        // progressDialog의 최대값을 총 데이터의 크기(파일 + 파일 정보)로 설정
        m_progressDialog->setMaximum(m_totalSize);

        /* 로그 tree widget에 채팅 로그 기록 */
        QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->messageTreeWidget);
        // Sender IP(Port)
        item->setText(0, QString::fromStdString(ip)+"("+QString::number(port)+")");
        // Sende ID(Name)
        item->setText(1, id+"("+ \
                      QString::fromStdString(m_clientIdNameMap[id.toStdString()]) +")");
        // filename
        item->setText(2, filename);
        // Receiver IP(Port)
        item->setText(3, m_fileServer->serverAddress().toString()+ \
                      "("+QString::number(PORT_NUMBER+1)+")" );
        // Receiver ID(name) = 10000(Admin)
        item->setText(4, QString("10000")+"("+tr("Admin")+")");
        // Time
        item->setText(5, QDateTime::currentDateTime().toString());
        item->setToolTip(2, filename);

        // 컨텐츠의 길이로 채팅 로그 tree widget의 헤더의 크기를 조정
        for(int i = 0; i < m_ui->messageTreeWidget->columnCount(); i++)
            m_ui->messageTreeWidget->resizeColumnToContents(i);

        m_ui->messageTreeWidget->addTopLevelItem(item);
        m_logThread->appendData(item);

        QFileInfo info(filename);                  // 파일의 정보를 가져옴
        std::string currentFileName = info.fileName().toStdString(); // 파일의 경로에서 이름만 뽑아옴
        m_file = new QFile(QString::fromStdString(currentFileName));         // 파일 생성
        m_file->open(QFile::WriteOnly|QFile::Truncate);
    }
    /* 파일 수신 중 */
    else {
        // 파일 데이터를 읽어서 저장
        m_inBlock = receivedSocket->readAll();
        m_byteReceived += m_inBlock.size(); // 수신한 누적 데이터의 크기
        m_file->write(m_inBlock);
        m_file->flush();
    }

    // progress dialog를 수신한 누적 데이터의 크기로 설정
    m_progressDialog->setValue(m_byteReceived);

    /* 파일을 다 수신하면 QFile 객체를 닫고 삭제 */
    if (m_byteReceived == m_totalSize) {
        qDebug() << QString("%1 receive completed").arg(filename);

        // 파일 수신 관련 변수 초기화
        m_inBlock.clear();
        m_byteReceived = 0;
        m_totalSize = 0;

        // progress dialog 닫기
        m_progressDialog->reset();
        m_progressDialog->hide();

        // file 객체 닫고 삭제
        m_file->close();
        delete m_file;
    }
}

/**
* @brief 새로운 연결이 들어오면 채팅을 위한 소켓을 생성하는 슬롯
*/
void ChatServerForm::clientConnect( ) const
{
    QTcpSocket *clientConnection = m_chatServer->nextPendingConnection();
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

    std::string ip = clientConnection->peerAddress().toString().toStdString();
    unsigned short port = clientConnection->peerPort();
    std::string strData = std::string(data);

    qDebug() << QString::fromStdString(ip) << " : " << type;


    switch(type) {
    case Chat_Login: { // 로그인
        std::string delim = ", ";
        size_t delimPos = strData.find(delim);
        std::string id = strData.substr(0, delimPos);
        std::string name = strData.substr(delimPos + delim.size());
        /* 고객 리스트에서 ID와 이름을 확인 */
        for(const auto& item : m_ui->clientTreeWidget-> \
                findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {

            // 고객 리스트에 ID와 이름이 있으면 로그인 허가
            if(item->text(2).toStdString() == name) {
                // 고객 리스트에서 고객의 상태를 online으로 변경
                if(item->text(0) != tr("Online")) {
                    item->setText(0, tr("Online"));
                    item->setIcon(0, QIcon(":/images/Blue-Circle.png"));

                    // clientIdSocketHash에 <id, socket> 추가
                    m_clientIdSocketMap[id] = clientConnection;
                    m_portClientMap[port] = id;

                    // 로그인을 허가한다는 메시지 전송
                    sendLoginResult(clientConnection, "permit");

                    // 관리자의 채팅창에서 고객의 상태를 online 변경
                    if(m_clientIdWindowMap.find(item->text(1).toStdString()) != m_clientIdWindowMap.end())
                        // 채팅창이 이미 만들어져 있으면 고객 상태 변경
                        m_clientIdWindowMap[item->text(1).toStdString()]->updateInfo("", tr("Online").toStdString());
                    else { // 채팅창이 만들어져 있지 않으면 새로 만들고 설정
                        ChatWindowForAdmin* w = new ChatWindowForAdmin(id, name, tr("Online").toStdString());
                        m_clientIdWindowMap[id] = w;
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
        for(const auto& item : m_ui->clientTreeWidget->findItems(QString::fromStdString(strData), Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경 */
            if(item->text(0) != tr("Chat in")) {
                item->setText(0, tr("Chat in"));
                item->setIcon(0, QIcon(":/images/Green-Circle.png"));

                if(m_clientIdWindowMap.find(item->text(1).toStdString()) != m_clientIdWindowMap.end())
                    m_clientIdWindowMap[item->text(1).toStdString()]->updateInfo("", tr("Chat in").toStdString());
            }
            m_clientIdSocketMap[strData] = clientConnection;
        }
        break;


    case Chat_Talk: { // 채팅 주고 받기
        if(m_clientIdWindowMap.find(m_portClientMap[port]) != m_clientIdWindowMap.end())
            m_clientIdWindowMap[m_portClientMap[port]]->receiveMessage(strData);

        /* 로그 tree widget에 채팅 로그 기록 */
        QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->messageTreeWidget);
        // Sender IP(Port)
        item->setText(0, QString::fromStdString(ip)+"("+QString::number(port)+")");
        // Sende ID(Name)
        item->setText(1, QString::fromStdString(m_portClientMap[port])+ \
                      "("+ QString::fromStdString(m_clientIdNameMap[m_portClientMap[port]])+")");
        // message
        item->setText(2, QString(data));
        // Receiver IP(Port)
        item->setText(3, m_chatServer->serverAddress().toString()+ \
                      "("+QString::number(PORT_NUMBER)+")" );
        // Receiver ID(name) = 10000(Admin)
        item->setText(4, QString("10000")+"("+tr("Admin")+")");
        // Time
        item->setText(5, QDateTime::currentDateTime().toString());
        item->setToolTip(2, QString(data));
        m_ui->messageTreeWidget->addTopLevelItem(item);

        for(int i = 0; i < m_ui->messageTreeWidget->columnCount(); i++)
            m_ui->messageTreeWidget->resizeColumnToContents(i);

        m_logThread->appendData(item);
    }
        break;


    case Chat_Out: // 고객이 채팅에서 나감
        for(const auto& item : m_ui->clientTreeWidget->findItems(QString::fromStdString(strData), Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경 */
            if(item->text(0) != tr("Online")) {
                item->setText(0, tr("Online"));
                item->setIcon(0, QIcon(":/images/Blue-Circle.png"));

                if(m_clientIdWindowMap.find(item->text(1).toStdString()) != m_clientIdWindowMap.end())
                    m_clientIdWindowMap[item->text(1).toStdString()]->updateInfo("", tr("Online").toStdString());
            }
        }
        break;


    case Chat_LogOut: // 고객이 log out
        for(const auto& item : m_ui->clientTreeWidget->findItems(QString::fromStdString(strData), Qt::MatchFixedString, 1)) {

            /* 고객 리스트와 관리자 채팅창에서 고객의 상태를 offline으로 변경 */
            if(item->text(0) != tr("Offline")) {
                item->setText(0, tr("Offline"));
                item->setIcon(0, QIcon(":/images/Red-Circle.png"));

                if(m_clientIdWindowMap.find(item->text(1).toStdString()) != m_clientIdWindowMap.end())
                    m_clientIdWindowMap[item->text(1).toStdString()]->updateInfo("", tr("Offline").toStdString());
            }
            m_clientIdSocketMap.erase(m_portClientMap[port]);
            m_portClientMap.erase(port);
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
        std::string id = m_portClientMap[clientConnection->peerPort()];
        for(const auto& item : m_ui->clientTreeWidget->findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
            qDebug() << item->text(2);
            item->setText(0, tr("Offline"));
            item->setIcon(0, QIcon(":/images/Red-Circle.png"));
            if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
                m_clientIdWindowMap[id]->updateInfo("", tr("Offline").toStdString());
        }

        /* 소켓 삭제 */
        m_clientIdSocketMap.erase(id);
        clientConnection->deleteLater();
    }
}

/**
* @brief 관리자의 채팅창을 여는 슬롯
*/
void ChatServerForm::openChatWindow()
{
    std::string id = m_ui->clientTreeWidget->currentItem()->text(1).toStdString();
    std::string state;


    if(m_clientIdWindowMap.find(id) == m_clientIdWindowMap.end()) { // 채팅창이 만들어져 있지 않으면 새로 만듦
        for(const auto& item : m_ui->clientTreeWidget->findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
            state = item->text(0).toStdString();
        }
        ChatWindowForAdmin* w = new ChatWindowForAdmin(id, m_clientIdNameMap[id], state);
        m_clientIdWindowMap[id] = w;
        w->show();
        assert(connect(w, SIGNAL(sendMessage(std::string,std::string)), this, SLOT(sendData(std::string,std::string))));
        assert(connect(w, SIGNAL(inviteClient(std::string)), this, SLOT(inviteClientInChatWindow(std::string))));
        assert(connect(w, SIGNAL(kickOutClient(std::string)), this, SLOT(kickOutInChatWindow(std::string))));
    }
    else {                                         // 채팅창이 이미 만들어져 있으면 열기만 함
        m_clientIdWindowMap[id]->showNormal();
        m_clientIdWindowMap[id]->activateWindow();
    }
}

/**
* @brief 고객을 채팅방에 초대하는 슬롯
*/
void ChatServerForm::inviteClient()
{
    if(m_ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객을 채팅방에 초대 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Invite;
    out.writeRawData("", 1020);

    // 현재 선택된 item에 표시된 ID와 해쉬로 부터 소켓을 가져옴
    std::string id = m_ui->clientTreeWidget->currentItem()->text(1).toStdString();
    QTcpSocket* sock = m_clientIdSocketMap[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경
    m_ui->clientTreeWidget->currentItem()->setText(0, tr("Chat in"));
    m_ui->clientTreeWidget->currentItem()->setIcon(0, QIcon(":/images/Green-Circle.png"));
    if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
        m_clientIdWindowMap[id]->updateInfo("", tr("Chat in").toStdString());
}

/**
* @brief 관리자의 채팅창에서 고객을 채팅방에 초대하는 슬롯
* @Param std::string id 초대할 고객의 id
*/
void ChatServerForm::inviteClientInChatWindow(const std::string id)
{
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Invite;
    out.writeRawData("", 1020);

    // ID와 해쉬로 부터 소켓을 가져옴
    QTcpSocket* sock = m_clientIdSocketMap[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 chat in으로 변경
    for(const auto& item : m_ui->clientTreeWidget-> \
            findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
        item->setText(0, tr("Chat in"));
        item->setIcon(0, QIcon(":/images/Green-Circle.png"));
    }
    if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
        m_clientIdWindowMap[id]->updateInfo("", tr("Chat in").toStdString());
}

/**
* @brief 고객을 채팅방에서 강퇴하는 슬롯
*/
void ChatServerForm::kickOut()
{
    if(m_ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객을 채팅방에서 강퇴 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_KickOut;
    out.writeRawData("", 1020);

    // 현재 선택된 item에 표시된 ID와 해쉬로 부터 소켓을 가져옴
    std::string id = m_ui->clientTreeWidget->currentItem()->text(1).toStdString();
    QTcpSocket* sock = m_clientIdSocketMap[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경
    m_ui->clientTreeWidget->currentItem()->setText(0, tr("Online"));
    m_ui->clientTreeWidget->currentItem()->setIcon(0, QIcon(":/images/Blue-Circle.png"));
    if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
        m_clientIdWindowMap[id]->updateInfo("", tr("Online").toStdString());
}

/**
* @brief 관리자의 채팅창에서 고객을 채팅방으로부터 강퇴하는 슬롯
* @Param std::string id 강퇴할 고객의 id
*/
void ChatServerForm::kickOutInChatWindow(const std::string id)
{
    /* 고객을 채팅방에서 강퇴 */
    QByteArray sendArray;
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_KickOut;
    out.writeRawData("", 1020);

    // ID와 해쉬로 부터 소켓을 가져옴
    QTcpSocket* sock = m_clientIdSocketMap[id];
    sock->write(sendArray);

    // 고객 리스트와 관리자 채팅창에서 고객의 상태를 online으로 변경
    for(const auto& item : m_ui->clientTreeWidget-> \
            findItems(QString::fromStdString(id), Qt::MatchFixedString, 1)) {
        item->setText(0, tr("Online"));
        item->setIcon(0, QIcon(":/images/Blue-Circle.png"));
    }
    if(m_clientIdWindowMap.find(id) != m_clientIdWindowMap.end())
        m_clientIdWindowMap[id]->updateInfo("", tr("Online").toStdString());
}

/**
* @brief 관리자가 고객에게 채팅을 전송하기 위한 슬롯
* @Param std::string 메시지를 받을 고객의 id
* @Param std::string str 전달할 메시지
*/
void ChatServerForm::sendData(const std::string id, const std::string str)
{

    if(m_clientIdSocketMap.find(id) == m_clientIdSocketMap.end())
        return;

    /* 메시지 전송 */
    QTcpSocket* sock = m_clientIdSocketMap[id];
    std::string data;
    QByteArray sendArray;
    sendArray.clear();
    QDataStream out(&sendArray, QIODevice::WriteOnly);
    out << Chat_Talk;
    data = "<font color=blue>" + tr("Admin").toStdString() + "</font> : " + str;
    out.writeRawData(data.c_str(), 1020);
    sock->write(sendArray);

    /* 로그 tree widget에 채팅 로그 기록 */
    QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->messageTreeWidget);
    // Sender IP(Port)
    item->setText(0, m_chatServer->serverAddress().toString()+\
                  "("+QString::number(PORT_NUMBER)+")" );
    // Sender ID(name) = 10000(Admin)
    item->setText(1, QString("10000")+"("+tr("Admin")+")");
    // message
    item->setText(2, QString::fromStdString(str));
    // Receiver IP(Port)
    item->setText(3, sock->peerAddress().toString()+ \
                  "("+QString::number(sock->peerPort())+")");
    // Receiver ID(name)
    item->setText(4, QString::fromStdString(id)+"("+ QString::fromStdString(m_clientIdNameMap[id])+")");
    // Time
    item->setText(5, QDateTime::currentDateTime().toString());
    item->setToolTip(2, QString::fromStdString(str));
    m_ui->messageTreeWidget->addTopLevelItem(item);

    for(int i = 0; i < m_ui->messageTreeWidget->columnCount(); i++)
        m_ui->messageTreeWidget->resizeColumnToContents(i);

    m_logThread->appendData(item);
}

/**
* @brief 고객 리스트 tree widget의 context menu 슬롯
* const QPoint &pos 우클릭한 위치
*/
void ChatServerForm::on_clientTreeWidget_customContextMenuRequested(const QPoint &pos) const
{
    if(m_ui->clientTreeWidget->currentItem() == nullptr)
        return;

    /* 고객 리스트 tree widget 위에서 우클릭한 위치에서 context menu 출력 */
    for(auto const& action : m_menu->actions()) {
        if(action->objectName() == "Open")        // 관리자 채팅창 열기(항상 활성화)
            action->setEnabled(true);
        else if(action->objectName() == "Invite") // 초대(고객이 online 일때만 활성화)
            action->setEnabled(m_ui->clientTreeWidget->currentItem()->text(0) == tr("Online"));
        else                                      // 강퇴(고객이 chat in 일때만 활성화)
            action->setEnabled(m_ui->clientTreeWidget->currentItem()->text(0) == tr("Chat in"));
    }
    QPoint globalPos = m_ui->clientTreeWidget->mapToGlobal(pos);
    m_menu->exec(globalPos);
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
