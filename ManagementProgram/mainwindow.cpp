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
    : QMainWindow(parent), ui(new Ui::MainWindow),
      clientForm(nullptr), productForm(nullptr), orderForm(nullptr), chatForm(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Client/Product/Order/Chat Management Program"));

    /* 고객, 제품 검색 기능을 제공하는 dialog 생성 */
    ClientDialog *clientDialog = new ClientDialog();    // 고객 검색
    ProductDialog *productDialog = new ProductDialog(); // 제품 검색

    // 고객 관리 객체 생성
    clientForm = new ClientManagerForm(this);
    assert(connect(clientForm, SIGNAL(destroyed()),
            clientForm, SLOT(deleteLater())));
    clientForm->setWindowTitle(tr("Client Info"));

    // 제품 관리 객체 생성
    productForm = new ProductManagerForm(this);
    assert(connect(productForm, SIGNAL(destroyed()),
            productForm, SLOT(deleteLater())));
    productForm->setWindowTitle(tr("Product Info"));

    // 주문 관리 객체 생성
    // 주문 관리 객체에서는 고객, 제품 검색 다이얼로그를 사용한다.
    orderForm = new OrderManagerForm(this, clientDialog, productDialog);
    assert(connect(orderForm, SIGNAL(destroyed()),
            orderForm, SLOT(deleteLater())));
    orderForm->setWindowTitle(tr("Order Info"));

    // 채팅 서버 객체 생성
    chatForm = new ChatServerForm(this);
    assert(connect(chatForm, SIGNAL(destroyed()),
            chatForm, SLOT(deleteLater())));
    chatForm->setWindowTitle(tr("Chat server"));

    /* 객체 간 signal과 slot을 connect */
    // 고객 검색 다이얼로그에서 검색어를 고객 관리 객체로 전달해줌
    assert(connect(clientDialog, SIGNAL(sendWord(std::string)), \
            clientForm, SLOT(receiveWord(std::string))));
    // 고객 관리 객체에서 검색 결과를 고객 검색 다이얼로그로 전달해줌
    assert(connect(clientForm, SIGNAL(sendClientToDialog(int,std::string,std::string,std::string)), \
            clientDialog, SLOT(receiveClientInfo(int,std::string,std::string,std::string))));

    // 제품 검색 다이얼로그에서 검색어를 제품 관리 객체로 전달해줌
    assert(connect(productDialog, SIGNAL(sendWord(std::string)), \
            productForm, SLOT(receiveWord(std::string))));
    // 제품 관리 객체에서 검색 결과를 제품 검색 다이얼로그로 전달해줌
    assert(connect(productForm, SIGNAL(sendProductToDialog(int,std::string,std::string,int,int)), \
            productDialog, SLOT(receiveProductInfo(int,std::string,std::string,int,int))));

    // 주문 관리 객체에서 정보를 받아올 고객의 id를 고객 관리 객체로 전달해줌
    assert(connect(orderForm, SIGNAL(sendClientId(int)), \
            clientForm, SLOT(receiveId(int))));
    // 고객 관리 객체에서 고객의 정보를 주문 관리 객체로 전달해줌
    assert(connect(clientForm, SIGNAL(sendClientToOrderManager(int,std::string,std::string,std::string)), \
            orderForm, SLOT(receiveClientInfo(int,std::string,std::string,std::string))));
    // 주문 관리 객체에서 정보를 받아올 제품의 id를 제품 관리 객체로 전달해줌
    assert(connect(orderForm, SIGNAL(sendProductId(int)), \
            productForm, SLOT(receiveId(int))));
    // 제품 관리 객체에서 제품의 정보를 주문 관리 객체로 전달해줌
    assert(connect(productForm, SIGNAL(sendProductToManager(int,std::string,std::string,int,int)), \
            orderForm, SLOT(receiveProductInfo(int,std::string,std::string,int,int))));

    // 고객 관리 객체에서 고객의 정보를 채팅 서버 객체로 전달해줌
    assert(connect(clientForm, SIGNAL(sendClientToChatServer(int,std::string)), \
            chatForm, SLOT(addClient(int,std::string))));

    // 고객 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(clientForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));
    // 제품 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(productForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));
    // 주문 관리 객체에서 mainwindow의 status bar에 메시지를 표시해줌
    assert(connect(orderForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int))));

    // 주문 관리 객체에서 제품 관리 객체를 통해 재고수량을 변경함
    assert(connect(orderForm, SIGNAL(sendStock(int,int)), \
            productForm, SLOT(setStock(int,int))));

    /* Mdi Area 설정 */
    QMdiSubWindow *cw = ui->mdiArea->addSubWindow(clientForm);
    ui->mdiArea->addSubWindow(productForm);
    ui->mdiArea->addSubWindow(orderForm);
    ui->mdiArea->addSubWindow(chatForm);
    ui->mdiArea->setActiveSubWindow(cw);

    /* 저장되어 있는 고객 리스트, 제품 리스트, 주문 리스트 불러오기 */
    clientForm->loadData();
    productForm->loadData();
    orderForm->loadData();



}

MainWindow::~MainWindow()
{
    delete ui;
    delete clientForm;
    delete productForm;
    delete orderForm;
    delete chatForm;

    QStringList list = QSqlDatabase::connectionNames();
    for(int i = 0; i < list.count(); ++i) {
        QSqlDatabase::removeDatabase(list[i]);
    }
}


void MainWindow::on_actionClient_triggered() const
{
    if(clientForm != nullptr) {
        clientForm->setFocus();
    }
}

void MainWindow::on_actionProduct_triggered() const
{
    if(productForm != nullptr) {
        productForm->setFocus();
    }
}

void MainWindow::on_actionOrder_triggered() const
{
    if(orderForm != nullptr) {
        orderForm->setFocus();
    }
}

void MainWindow::on_actionChat_triggered() const
{
    if(chatForm != nullptr) {
        chatForm->setFocus();
    }
}


