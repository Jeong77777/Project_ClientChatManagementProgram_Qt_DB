#include "widget.h"
#include "ui_widget.h"

#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDataStream>
#include <QTcpSocket>
#include <QApplication>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QProgressDialog>
#include <cassert>

#include "logthread.h"


#define BLOCK_SIZE      1024

/**
 * @brief 생성자, 소켓 설정, 버튼 설정, 입력 창 설정
 */
Widget::Widget(QWidget *parent)
    : QWidget(parent), PORT_NUMBER(8000),
      ui(new Ui::Widget),
      clientSocket(nullptr), fileClient(nullptr), progressDialog(nullptr),
      file(nullptr), loadSize(0), byteToWrite(0), totalSize(0), outBlock(),
      isSent(false), logThread(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Chat Program for Client"));

    /* IP주소, PORT 번호 입력 칸에 대한 정규 표현식 설정 */
    ui->serverAddress->setText("127.0.0.1");
    QRegularExpression re("^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QRegularExpressionValidator validator(re);
    ui->serverAddress->setPlaceholderText("Server IP Address");
    ui->serverAddress->setValidator(&validator);

    ui->serverPort->setText(QString::number(PORT_NUMBER));
    ui->serverPort->setInputMask("00000;_");
    ui->serverPort->setPlaceholderText("Server Port No");


    /* 메시지 입력 창에서 enter 키를 누르면 메시지가 보내지고 입력 창이 clear 되도록 connect */
    assert(connect(ui->inputLine, SIGNAL(returnPressed()), SLOT(sendData())));
    assert(connect(ui->inputLine, SIGNAL(returnPressed()), ui->inputLine, SLOT(clear())));

    /* send 버튼을 누르면 메시지가 보내지고 입력 창이 clear 되도록 connect */
    assert(connect(ui->sentButton, SIGNAL(clicked()), SLOT(sendData())));
    assert(connect(ui->sentButton, SIGNAL(clicked()), ui->inputLine, SLOT(clear())));

    /* 프로그램 실행 시 초기에 메시지 입력 창과 send 버튼은 disabled로 설정 */
    ui->inputLine->setDisabled(true);
    ui->sentButton->setDisabled(true);

    ui->logOutButton->setDisabled(true);

    /* 파일 전송 버튼을 누르면 파일 전송이 시작 되도록 설정 */
    assert(connect(ui->fileButton, SIGNAL(clicked()), SLOT(sendFile())));
    ui->fileButton->setDisabled(true);

    /* 채팅을 위한 소켓 생성 */
    clientSocket = new QTcpSocket(this);
    assert(connect(clientSocket, &QAbstractSocket::errorOccurred, this,
            [=]{ qDebug() << clientSocket->errorString(); }));
    assert(connect(clientSocket, SIGNAL(readyRead()), SLOT(receiveData())));
    assert(connect(clientSocket, SIGNAL(disconnected()), SLOT(disconnect())));

    /* 파일 전송을 위한 소켓 생성 */
    fileClient = new QTcpSocket(this);
    // 파일 전송시 여러 번 나눠서 전송
    assert(connect(fileClient, SIGNAL(bytesWritten(qint64)), SLOT(goOnSend(qint64))));

    /* 파일 전송 진행 상태를 나타내는 progress dialog 초기화 */
    progressDialog = new QProgressDialog(0);
    progressDialog->setAutoClose(true);
    progressDialog->reset();

    /* connect button 설정 */
    assert(connect(ui->connectButton, &QPushButton::clicked, this,
            [=]{
        if(ui->connectButton->text() == tr("Log In")) {
            ui->connectButton->setDisabled(true);
            ui->id->setReadOnly(true);
            ui->name->setReadOnly(true);
            clientSocket->connectToHost(ui->serverAddress->text( ),
                                        ui->serverPort->text( ).toInt( ));
            clientSocket->waitForConnected();
            // 로그인을 시도 할때는 id와 이름을 서버로 전송함
            sendProtocol(Chat_Login, (ui->id->text() + ", " \
                                      + ui->name->text()).toStdString().data());
        } else if(ui->connectButton->text() == tr("Chat in"))  {
            sendProtocol(Chat_In, ui->id->text().toStdString().data());
            ui->connectButton->setText(tr("Chat Out"));
            ui->inputLine->setEnabled(true);
            ui->sentButton->setEnabled(true);
            ui->fileButton->setEnabled(true);
        } else if(ui->connectButton->text() == tr("Chat Out"))  {
            sendProtocol(Chat_Out, ui->id->text().toStdString().data());
            ui->connectButton->setText(tr("Chat in"));
            ui->inputLine->setDisabled(true);
            ui->sentButton->setDisabled(true);
            ui->fileButton->setDisabled(true);
        }
    } ));

    /* 로그아웃 기능 */
    assert(connect(ui->logOutButton, &QPushButton::clicked, this,
            [=]{
        ui->connectButton->setText(tr("Log In"));
        ui->logOutButton->setDisabled(true);

        // 채팅 로그 thread 종료
        if(logThread != nullptr) {
            logThread->saveData();
            logThread->terminate();
        }

        sendProtocol(Chat_LogOut, ui->id->text().toStdString().data());
        ui->fileButton->setDisabled(true);
        ui->sentButton->setDisabled(true);
        ui->message->clear();
        ui->id->setReadOnly(false);
        ui->name->setReadOnly(false);
    } ));

    logThread = nullptr;
}

Widget::~Widget()
{
    clientSocket->close();
    fileClient->close();
    logThread->saveData();
    logThread->terminate();

    clientSocket->deleteLater(); clientSocket = nullptr;
    fileClient->deleteLater(); fileClient = nullptr;
    delete progressDialog; progressDialog = nullptr;
    logThread->deleteLater(); logThread = nullptr;
    delete ui; ui = nullptr;
}

/**
 * @brief 창이 닫힐 때 서버에 로그아웃 메시지를 보내고 종료
 */
void Widget::closeEvent(QCloseEvent*)
{
    sendProtocol(Chat_LogOut, ui->id->text().toStdString().data());
    clientSocket->disconnectFromHost();
    if(clientSocket->state() != QAbstractSocket::UnconnectedState)
        clientSocket->waitForDisconnected();
}

/**
 * @brief 관리자(서버)로부터 메시지를 받기 위한 슬롯
 */
void Widget::receiveData( )
{
    QTcpSocket *clientSocket = dynamic_cast<QTcpSocket *>(sender( ));
    if (clientSocket->bytesAvailable( ) > BLOCK_SIZE) return;
    QByteArray bytearray = clientSocket->read(BLOCK_SIZE);

    Chat_Status type;       // 채팅의 목적
    char data[1020];        // 수신되는 메시지
    memset(data, 0, 1020);

    QDataStream in(&bytearray, QIODevice::ReadOnly);
    in.device()->seek(0);
    in >> type;                  // 패킷의 타입
    in.readRawData(data, 1020);  // 실제 데이터

    switch(type) {
    case Chat_Login: // 로그인 결과
        if(0 == strcmp(data, "permit")) {
            ui->connectButton->setText(tr("Chat in"));
            ui->connectButton->setEnabled(true);
            ui->logOutButton->setEnabled(true);

            // 이전 채팅 내용 불러오기
            loadData(ui->id->text().toInt(), ui->name->text().toStdString());
            // 채팅 로그 기록 thread 생성 및 시작
            logThread = new LogThread(ui->id->text().toInt(), ui->name->text().toStdString());
            logThread->start();
        }
        else {
            ui->connectButton->setEnabled(true);
            QMessageBox::critical(this, tr("Chatting Client"), \
                                  tr("Login failed.\nCustomer information is invalid."));
            ui->id->setReadOnly(false);
            ui->name->setReadOnly(false);
        }
        break;


    case Chat_Talk: // 채팅 주고 받기
        ui->message->append(QString(data)); // 받은 메시지를 화면에 표시
        // 버튼, 입력 창 상태 변경
        ui->inputLine->setEnabled(true);
        ui->sentButton->setEnabled(true);
        ui->fileButton->setEnabled(true);

        // 채팅 로그 기록 thread에 채팅 내용 추가
        logThread->appendData(std::string(data) + " | " + QDateTime::currentDateTime().toString().toStdString());
        break;


    case Chat_KickOut: // 강퇴 당함
        QMessageBox::information(this, tr("Chatting Client"), \
                                 tr("Kick out from Server"));
        // 버튼, 입력 창 상태 변경
        ui->inputLine->setDisabled(true);
        ui->sentButton->setDisabled(true);
        ui->fileButton->setDisabled(true);
        ui->connectButton->setText(tr("Chat in"));
        break;


    case Chat_Invite: // 초대를 받음
        QMessageBox::information(this, tr("Chatting Client"), \
                                 tr("Invited from Server"));
        // 버튼, 입력 창 상태 변경
        ui->inputLine->setEnabled(true);
        ui->sentButton->setEnabled(true);
        ui->fileButton->setEnabled(true);
        ui->connectButton->setText(tr("Chat Out"));
        break;


    default:
        break;
    };
}

/**
 * @brief 관리자(서버)로 메시지를 보내기 위한 슬롯
 */
void Widget::sendData() const
{
    std::string str = ui->inputLine->text().toStdString();
    if(str.length()) {
        QByteArray bytearray;
        bytearray = QString::fromStdString(str).toUtf8( );
        // 화면에 표시 : 앞에 'me(나)'라고 추가
        ui->message->append("<font color=red>" + tr("Me") + "</font> : " + QString::fromStdString(str));
        sendProtocol(Chat_Talk, bytearray.data());

        // 채팅 로그 기록 thread에 채팅 내용 추가
        logThread->appendData("<font color=red>" + tr("Me").toStdString() + "</font> : " + str \
                              + " | " + QDateTime::currentDateTime().toString().toStdString());
    }
}

/**
 * @brief 서버와의 연결이 끊어졌을 때의 슬롯
 */
void Widget::disconnect()
{
    if(ui->connectButton->text() != tr("Log In"))
        QMessageBox::critical(this, tr("Chatting Client"), \
                              tr("Disconnect from Server"));

    // 채팅 로그 thread 종료
    if(logThread != nullptr) {
        logThread->saveData();
        logThread->terminate();
    }

    // 버튼, 입력 창 상태 변경
    ui->inputLine->setEnabled(false);
    ui->id->setReadOnly(false);
    ui->name->setReadOnly(false);
    ui->sentButton->setEnabled(false);
    ui->fileButton->setEnabled(false);
    ui->connectButton->setText(tr("Log In"));
    ui->logOutButton->setDisabled(true);
    isSent = false;
}

/**
 * @brief 프로토콜을 생성해서 서버로 전송
 * @param Chat_Status type 채팅의 목적
 * @param char* data 실제 전송할 데이터
 * @param int size
 */
void Widget::sendProtocol(const Chat_Status type, const char *data, const int size) const
{
    /* 소켓으로 보낼 데이터를 채우고 서버로 전송 */
    QByteArray dataArray;
    QDataStream out(&dataArray, QIODevice::WriteOnly);
    out.device()->seek(0);
    out << type;
    out.writeRawData(data, size);
    clientSocket->write(dataArray);
    clientSocket->flush();
    while(clientSocket->waitForBytesWritten());
}

/**
 * @brief 관리자(서버)로 파일을 보내기 위한 슬롯
 */
void Widget::sendFile()
{
    /* 파일 전송 시작 */
    loadSize = 0;
    byteToWrite = 0;
    totalSize = 0;
    outBlock.clear();

    QString filename = QFileDialog::getOpenFileName(this);
    if(filename.length()) {

        // 전송할 파일 가져오기
        file = new QFile(filename);
        file->open(QFile::ReadOnly);

        qDebug() << QString("file %1 is opened").arg(filename);
        progressDialog->setValue(0);

        if (!isSent) { // 파일 서버와의 연결이 되어있지 않으면 연결
            fileClient->connectToHost(ui->serverAddress->text( ),
                                      ui->serverPort->text( ).toInt( ) + 1);
            isSent = true;
        }

        // 전송할 남은 데이터의 크기 = 총 데이터의 크기 = 전송할 파일 사이즈
        byteToWrite = totalSize = file->size();
        loadSize = 1024; // 데이터 전송 단위 크기

        QDataStream out(&outBlock, QIODevice::WriteOnly);
        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈, 파일이름, 보내는사람이름
        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈를 저장할 공간을 qint64(0)으로 확보
        out << qint64(0) << qint64(0) << filename << ui->id->text();

        totalSize += outBlock.size();   // 총 데이터의 크기(파일 + 파일 정보)
        byteToWrite += outBlock.size(); // 전송할 남은 데이터의 크기

        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈를 저장
        out.device()->seek(0);
        out << totalSize << qint64(outBlock.size());

        // 소켓으로 전송
        fileClient->write(outBlock);

        // progress dialog 조정
        progressDialog->setMaximum(totalSize);
        progressDialog->setValue(totalSize-byteToWrite);
        progressDialog->show();
    }
    qDebug() << QString("Sending file %1").arg(filename);
}

/**
 * @brief 파일을 여러 번 나눠서 전송하기 위한 슬롯
 * @param qint64 numBytes 이전에 보낸 데이터의 사이즈
 */
void Widget::goOnSend(const qint64 numBytes)
{
    byteToWrite -= numBytes; // 전송할 남은 데이터의 크기

    /* 파일 전송 */
    outBlock = file->read(qMin(byteToWrite, numBytes));
    fileClient->write(outBlock);

    /* 파일 전송 진행 상태 progress dialog 조정 */
    progressDialog->setMaximum(totalSize);
    progressDialog->setValue(totalSize-byteToWrite);

    if (byteToWrite == 0) { // 파일 전송이 완료 되었을 때
        qDebug("File sending completed!");
        progressDialog->reset();
        file->close();
        delete file; file = nullptr;
    }
}

/**
 * @brief 이전 채팅 내용 불러오기
 * @param int id 고객ID
 * @param std::string name 이름
 */
void Widget::loadData(int id, std::string name) const
{
    // 로그 파일 검색 log_(ID)_(이름).txt
    QFile file("log_" + QString::number(id)+"_"+QString::fromStdString(name)+".txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // 이전 채팅 내용 불러오기
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QList<QString> row = line.split(" | ");
        if(row.size()) {
            ui->message->append(row[0]);
        }
    }
    file.close( );
    return;
}
