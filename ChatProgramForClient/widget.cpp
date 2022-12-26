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
      m_ui(new Ui::Widget),
      m_clientSocket(nullptr), m_fileClient(nullptr), m_progressDialog(nullptr),
      m_file(nullptr), m_loadSize(0), m_byteToWrite(0), m_totalSize(0), m_outBlock(),
      m_isSent(false), m_logThread(nullptr)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Chat Program for Client"));

    /* IP주소, PORT 번호 입력 칸에 대한 정규 표현식 설정 */
    m_ui->serverAddress->setText("127.0.0.1");
    QRegularExpression re("^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QRegularExpressionValidator validator(re);
    m_ui->serverAddress->setPlaceholderText("Server IP Address");
    m_ui->serverAddress->setValidator(&validator);

    m_ui->serverPort->setText(QString::number(PORT_NUMBER));
    m_ui->serverPort->setInputMask("00000;_");
    m_ui->serverPort->setPlaceholderText("Server Port No");


    /* 메시지 입력 창에서 enter 키를 누르면 메시지가 보내지고 입력 창이 clear 되도록 connect */
    assert(connect(m_ui->inputLine, SIGNAL(returnPressed()), SLOT(sendData())));
    assert(connect(m_ui->inputLine, SIGNAL(returnPressed()), m_ui->inputLine, SLOT(clear())));

    /* send 버튼을 누르면 메시지가 보내지고 입력 창이 clear 되도록 connect */
    assert(connect(m_ui->sentButton, SIGNAL(clicked()), SLOT(sendData())));
    assert(connect(m_ui->sentButton, SIGNAL(clicked()), m_ui->inputLine, SLOT(clear())));

    /* 프로그램 실행 시 초기에 메시지 입력 창과 send 버튼은 disabled로 설정 */
    m_ui->inputLine->setDisabled(true);
    m_ui->sentButton->setDisabled(true);

    m_ui->logOutButton->setDisabled(true);

    /* 파일 전송 버튼을 누르면 파일 전송이 시작 되도록 설정 */
    assert(connect(m_ui->fileButton, SIGNAL(clicked()), SLOT(sendFile())));
    m_ui->fileButton->setDisabled(true);

    /* 채팅을 위한 소켓 생성 */
    m_clientSocket = new QTcpSocket(this);
    assert(connect(m_clientSocket, &QAbstractSocket::errorOccurred, this,
            [=]{ qDebug() << m_clientSocket->errorString(); }));
    assert(connect(m_clientSocket, SIGNAL(readyRead()), SLOT(receiveData())));
    assert(connect(m_clientSocket, SIGNAL(disconnected()), SLOT(disconnect())));

    /* 파일 전송을 위한 소켓 생성 */
    m_fileClient = new QTcpSocket(this);
    // 파일 전송시 여러 번 나눠서 전송
    assert(connect(m_fileClient, SIGNAL(bytesWritten(qint64)), SLOT(goOnSend(qint64))));

    /* 파일 전송 진행 상태를 나타내는 progress dialog 초기화 */
    m_progressDialog = new QProgressDialog(0);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->reset();

    /* connect button 설정 */
    assert(connect(m_ui->connectButton, &QPushButton::clicked, this,
            [=]{
        if(m_ui->connectButton->text() == tr("Log In")) {
            m_ui->connectButton->setDisabled(true);
            m_ui->id->setReadOnly(true);
            m_ui->name->setReadOnly(true);
            m_clientSocket->connectToHost(m_ui->serverAddress->text( ),
                                        m_ui->serverPort->text( ).toInt( ));
            m_clientSocket->waitForConnected();
            // 로그인을 시도 할때는 id와 이름을 서버로 전송함
            sendProtocol(Chat_Login, (m_ui->id->text() + ", " \
                                      + m_ui->name->text()).toStdString().data());
        } else if(m_ui->connectButton->text() == tr("Chat in"))  {
            sendProtocol(Chat_In, m_ui->id->text().toStdString().data());
            m_ui->connectButton->setText(tr("Chat Out"));
            m_ui->inputLine->setEnabled(true);
            m_ui->sentButton->setEnabled(true);
            m_ui->fileButton->setEnabled(true);
        } else if(m_ui->connectButton->text() == tr("Chat Out"))  {
            sendProtocol(Chat_Out, m_ui->id->text().toStdString().data());
            m_ui->connectButton->setText(tr("Chat in"));
            m_ui->inputLine->setDisabled(true);
            m_ui->sentButton->setDisabled(true);
            m_ui->fileButton->setDisabled(true);
        }
    } ));

    /* 로그아웃 기능 */
    assert(connect(m_ui->logOutButton, &QPushButton::clicked, this,
            [=]{
        m_ui->connectButton->setText(tr("Log In"));
        m_ui->logOutButton->setDisabled(true);

        // 채팅 로그 thread 종료
        if(m_logThread != nullptr) {
            m_logThread->saveData();
            m_logThread->terminate();
        }

        sendProtocol(Chat_LogOut, m_ui->id->text().toStdString().data());
        m_ui->fileButton->setDisabled(true);
        m_ui->sentButton->setDisabled(true);
        m_ui->message->clear();
        m_ui->id->setReadOnly(false);
        m_ui->name->setReadOnly(false);
    } ));

    m_logThread = nullptr;
}

Widget::~Widget()
{
    m_clientSocket->close();
    m_fileClient->close();
    m_logThread->saveData();
    m_logThread->terminate();

    m_clientSocket->deleteLater(); m_clientSocket = nullptr;
    m_fileClient->deleteLater(); m_fileClient = nullptr;
    delete m_progressDialog; m_progressDialog = nullptr;
    m_logThread->deleteLater(); m_logThread = nullptr;
    delete m_ui; m_ui = nullptr;
}

/**
 * @brief 창이 닫힐 때 서버에 로그아웃 메시지를 보내고 종료
 */
void Widget::closeEvent(QCloseEvent*)
{
    sendProtocol(Chat_LogOut, m_ui->id->text().toStdString().data());
    m_clientSocket->disconnectFromHost();
    if(m_clientSocket->state() != QAbstractSocket::UnconnectedState)
        m_clientSocket->waitForDisconnected();
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
            m_ui->connectButton->setText(tr("Chat in"));
            m_ui->connectButton->setEnabled(true);
            m_ui->logOutButton->setEnabled(true);

            // 이전 채팅 내용 불러오기
            loadData(m_ui->id->text().toInt(), m_ui->name->text().toStdString());
            // 채팅 로그 기록 thread 생성 및 시작
            m_logThread = new LogThread(m_ui->id->text().toInt(), m_ui->name->text().toStdString());
            m_logThread->start();
        }
        else {
            m_ui->connectButton->setEnabled(true);
            QMessageBox::critical(this, tr("Chatting Client"), \
                                  tr("Login failed.\nCustomer information is invalid."));
            m_ui->id->setReadOnly(false);
            m_ui->name->setReadOnly(false);
        }
        break;


    case Chat_Talk: // 채팅 주고 받기
        m_ui->message->append(QString(data)); // 받은 메시지를 화면에 표시
        // 버튼, 입력 창 상태 변경
        m_ui->inputLine->setEnabled(true);
        m_ui->sentButton->setEnabled(true);
        m_ui->fileButton->setEnabled(true);

        // 채팅 로그 기록 thread에 채팅 내용 추가
        m_logThread->appendData(std::string(data) + " | " + QDateTime::currentDateTime().toString().toStdString());
        break;


    case Chat_KickOut: // 강퇴 당함
        QMessageBox::information(this, tr("Chatting Client"), \
                                 tr("Kick out from Server"));
        // 버튼, 입력 창 상태 변경
        m_ui->inputLine->setDisabled(true);
        m_ui->sentButton->setDisabled(true);
        m_ui->fileButton->setDisabled(true);
        m_ui->connectButton->setText(tr("Chat in"));
        break;


    case Chat_Invite: // 초대를 받음
        QMessageBox::information(this, tr("Chatting Client"), \
                                 tr("Invited from Server"));
        // 버튼, 입력 창 상태 변경
        m_ui->inputLine->setEnabled(true);
        m_ui->sentButton->setEnabled(true);
        m_ui->fileButton->setEnabled(true);
        m_ui->connectButton->setText(tr("Chat Out"));
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
    std::string str = m_ui->inputLine->text().toStdString();
    if(str.length()) {
        QByteArray bytearray;
        bytearray = QString::fromStdString(str).toUtf8( );
        // 화면에 표시 : 앞에 'me(나)'라고 추가
        m_ui->message->append("<font color=red>" + tr("Me") + "</font> : " + QString::fromStdString(str));
        sendProtocol(Chat_Talk, bytearray.data());

        // 채팅 로그 기록 thread에 채팅 내용 추가
        m_logThread->appendData("<font color=red>" + tr("Me").toStdString() + "</font> : " + str \
                              + " | " + QDateTime::currentDateTime().toString().toStdString());
    }
}

/**
 * @brief 서버와의 연결이 끊어졌을 때의 슬롯
 */
void Widget::disconnect()
{
    if(m_ui->connectButton->text() != tr("Log In"))
        QMessageBox::critical(this, tr("Chatting Client"), \
                              tr("Disconnect from Server"));

    // 채팅 로그 thread 종료
    if(m_logThread != nullptr) {
        m_logThread->saveData();
        m_logThread->terminate();
    }

    // 버튼, 입력 창 상태 변경
    m_ui->inputLine->setEnabled(false);
    m_ui->id->setReadOnly(false);
    m_ui->name->setReadOnly(false);
    m_ui->sentButton->setEnabled(false);
    m_ui->fileButton->setEnabled(false);
    m_ui->connectButton->setText(tr("Log In"));
    m_ui->logOutButton->setDisabled(true);
    m_isSent = false;
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
    m_clientSocket->write(dataArray);
    m_clientSocket->flush();
    while(m_clientSocket->waitForBytesWritten());
}

/**
 * @brief 관리자(서버)로 파일을 보내기 위한 슬롯
 */
void Widget::sendFile()
{
    /* 파일 전송 시작 */
    m_loadSize = 0;
    m_byteToWrite = 0;
    m_totalSize = 0;
    m_outBlock.clear();

    QString filename = QFileDialog::getOpenFileName(this);
    if(filename.length()) {

        // 전송할 파일 가져오기
        m_file = new QFile(filename);
        m_file->open(QFile::ReadOnly);

        qDebug() << QString("file %1 is opened").arg(filename);
        m_progressDialog->setValue(0);

        if (!m_isSent) { // 파일 서버와의 연결이 되어있지 않으면 연결
            m_fileClient->connectToHost(m_ui->serverAddress->text( ),
                                      m_ui->serverPort->text( ).toInt( ) + 1);
            m_isSent = true;
        }

        // 전송할 남은 데이터의 크기 = 총 데이터의 크기 = 전송할 파일 사이즈
        m_byteToWrite = m_totalSize = m_file->size();
        m_loadSize = 1024; // 데이터 전송 단위 크기

        QDataStream out(&m_outBlock, QIODevice::WriteOnly);
        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈, 파일이름, 보내는사람이름
        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈를 저장할 공간을 qint64(0)으로 확보
        out << qint64(0) << qint64(0) << filename << m_ui->id->text();

        m_totalSize += m_outBlock.size();   // 총 데이터의 크기(파일 + 파일 정보)
        m_byteToWrite += m_outBlock.size(); // 전송할 남은 데이터의 크기

        // 총 데이터의 크기(파일 + 파일 정보), 블록 사이즈를 저장
        out.device()->seek(0);
        out << m_totalSize << qint64(m_outBlock.size());

        // 소켓으로 전송
        m_fileClient->write(m_outBlock);

        // progress dialog 조정
        m_progressDialog->setMaximum(m_totalSize);
        m_progressDialog->setValue(m_totalSize-m_byteToWrite);
        m_progressDialog->show();
    }
    qDebug() << QString("Sending file %1").arg(filename);
}

/**
 * @brief 파일을 여러 번 나눠서 전송하기 위한 슬롯
 * @param qint64 numBytes 이전에 보낸 데이터의 사이즈
 */
void Widget::goOnSend(const qint64 numBytes)
{
    m_byteToWrite -= numBytes; // 전송할 남은 데이터의 크기

    /* 파일 전송 */
    m_outBlock = m_file->read(qMin(m_byteToWrite, numBytes));
    m_fileClient->write(m_outBlock);

    /* 파일 전송 진행 상태 progress dialog 조정 */
    m_progressDialog->setMaximum(m_totalSize);
    m_progressDialog->setValue(m_totalSize-m_byteToWrite);

    if (m_byteToWrite == 0) { // 파일 전송이 완료 되었을 때
        qDebug("File sending completed!");
        m_progressDialog->reset();
        m_file->close();
        delete m_file; m_file = nullptr;
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
            m_ui->message->append(row[0]);
        }
    }
    file.close( );
    return;
}
