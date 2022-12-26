#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clientmanagerform.h"
#include "productmanagerform.h"
#include "ordermanagerform.h"
#include "clientdialog.h"
#include "productdialog.h"
#include "chatserverform.h"

#include <QMdiSubWindow>
#include <QSqlDatabase>
#include <cassert>
#include <string>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_ui(new Ui::MainWindow),
      m_clientForm(nullptr), m_productForm(nullptr), m_orderForm(nullptr), m_chatForm(nullptr),
      m_clientDialog(nullptr), m_productDialog(nullptr)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Client/Product/Order/Chat Management Program"));

    /* 고객, 제품 검색 기능을 제공하는 dialog 생성 */
    m_clientDialog = new ClientDialog();    // 고객 검색
    m_productDialog = new ProductDialog(); // 제품 검색

    // 고객 관리 객체 생성
    m_clientForm = new ClientManagerForm(this);
    assert(connect(m_clientForm, SIGNAL(destroyed()),
            m_clientForm, SLOT(deleteLater())));
    m_clientForm->setWindowTitle(tr("Client Info"));

    // 제품 관리 객체 생성
    m_productForm = new ProductManagerForm(this);
    assert(connect(m_productForm, SIGNAL(destroyed()),
            m_productForm, SLOT(deleteLater())));
    m_productForm->setWindowTitle(tr("Product Info"));

    // 주문 관리 객체 생성
    // 주문 관리 객체에서는 고객, 제품 검색 다이얼로그를 사용한다.
    m_orderForm = new OrderManagerForm(this, m_clientDialog, m_productDialog);
    assert(connect(m_orderForm, SIGNAL(destroyed()),
            m_orderForm, SLOT(deleteLater())));
    m_orderForm->setWindowTitle(tr("Order Info"));

    // 채팅 서버 객체 생성
    m_chatForm = new ChatServerForm(this);
    assert(connect(m_chatForm, SIGNAL(destroyed()),
            m_chatForm, SLOT(deleteLater())));
    m_chatForm->setWindowTitle(tr("Chat server"));

    /* 객체 간 signal과 slot을 connect */
    // 고객 검색 다이얼로그에서 검색어를 고객 관리 객체로 전달해줌
    assert(connect(m_clientDialog, SIGNAL(sendWord(std::string)), \
            m_clientForm, SLOT(receiveWord(std::string))));
    // 고객 관리 객체에서 검색 결과를 고객 검색 다이얼로그로 전달해줌
    assert(connect(m_clientForm, SIGNAL(sendClientToDialog(int,std::string,std::string,std::string)), \
            m_clientDialog, SLOT(receiveClientInfo(int,std::string,std::string,std::string))));

    // 제품 검색 다이얼로그에서 검색어를 제품 관리 객체로 전달해줌
    assert(connect(m_productDialog, SIGNAL(sendWord(std::string)), \
            m_productForm, SLOT(receiveWord(std::string))));
    // 제품 관리 객체에서 검색 결과를 제품 검색 다이얼로그로 전달해줌
    assert(connect(m_productForm, SIGNAL(sendProductToDialog(int,std::string,std::string,int,int)), \
            m_productDialog, SLOT(receiveProductInfo(int,std::string,std::string,int,int))));

    // 주문 관리 객체에서 정보를 받아올 고객의 id를 고객 관리 객체로 전달해줌
    assert(connect(m_orderForm, SIGNAL(sendClientId(int)), \
            m_clientForm, SLOT(receiveId(int))));
    // 고객 관리 객체에서 고객의 정보를 주문 관리 객체로 전달해줌
    assert(connect(m_clientForm, SIGNAL(sendClientToOrderManager(int,std::string,std::string,std::string)), \
            m_orderForm, SLOT(receiveClientInfo(int,std::string,std::string,std::string))));
    // 주문 관리 객체에서 정보를 받아올 제품의 id를 제품 관리 객체로 전달해줌
    assert(connect(m_orderForm, SIGNAL(sendProductId(int)), \
            m_productForm, SLOT(receiveId(int))));
    // 제품 관리 객체에서 제품의 정보를 주문 관리 객체로 전달해줌
    assert(connect(m_productForm, SIGNAL(sendProductToManager(int,std::string,std::string,int,int)), \
            m_orderForm, SLOT(receiveProductInfo(int,std::string,std::string,int,int))));

    // 고객 관리 객체에서 고객의 정보를 채팅 서버 객체로 전달해줌
    assert(connect(m_clientForm, SIGNAL(sendClientToChatServer(int,std::string)), \
            m_chatForm, SLOT(addClient(int,std::string))));

    // 고객 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(m_clientForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));
    // 제품 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(m_productForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));
    // 주문 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(m_orderForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));

    // 주문 관리 객체에서 제품 관리 객체를 통해 재고수량을 변경함
    assert(connect(m_orderForm, SIGNAL(sendStock(int,int)), \
            m_productForm, SLOT(setStock(int,int))));

    /* Mdi Area 설정 */
    QMdiSubWindow *cw = m_ui->mdiArea->addSubWindow(m_clientForm);
    m_ui->mdiArea->addSubWindow(m_productForm);
    m_ui->mdiArea->addSubWindow(m_orderForm);
    m_ui->mdiArea->addSubWindow(m_chatForm);
    m_ui->mdiArea->setActiveSubWindow(cw);

    /* 저장되어 있는 고객 리스트, 제품 리스트, 주문 리스트 불러오기 */
    m_clientForm->loadData();
    m_productForm->loadData();
    m_orderForm->loadData();



}

MainWindow::~MainWindow()
{
    delete m_clientDialog; m_clientDialog = nullptr;
    delete m_productDialog; m_productDialog = nullptr;
    delete m_clientForm; m_clientForm = nullptr;
    delete m_productForm; m_productForm = nullptr;
    delete m_orderForm; m_orderForm = nullptr;
    delete m_chatForm; m_chatForm = nullptr;
    delete m_ui; m_ui = nullptr;

    QStringList list = QSqlDatabase::connectionNames();
    for(int i = 0; i < list.count(); ++i) {
        QSqlDatabase::removeDatabase(list[i]);
    }
}


void MainWindow::on_actionClient_triggered() const
{
    if(m_clientForm != nullptr) {
        m_clientForm->setFocus();
    }
}

void MainWindow::on_actionProduct_triggered() const
{
    if(m_productForm != nullptr) {
        m_productForm->setFocus();
    }
}

void MainWindow::on_actionOrder_triggered() const
{
    if(m_orderForm != nullptr) {
        m_orderForm->setFocus();
    }
}

void MainWindow::on_actionChat_triggered() const
{
    if(m_chatForm != nullptr) {
        m_chatForm->setFocus();
    }
}


