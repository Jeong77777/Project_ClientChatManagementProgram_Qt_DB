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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(tr("Client/Product/Order/Chat Management Program"));

    /* 고객, 제품 검색 기능을 제공하는 dialog 생성 */
    ClientDialog *clientDialog = new ClientDialog();    // 고객 검색
    ProductDialog *productDialog = new ProductDialog(); // 제품 검색

    // 고객 관리 객체 생성
    clientForm = new ClientManagerForm(this);
    connect(clientForm, SIGNAL(destroyed()),
            clientForm, SLOT(deleteLater()));
    clientForm->setWindowTitle(tr("Client Info"));

    // 제품 관리 객체 생성
    productForm = new ProductManagerForm(this);
    connect(productForm, SIGNAL(destroyed()),
            productForm, SLOT(deleteLater()));
    productForm->setWindowTitle(tr("Product Info"));

    // 주문 관리 객체 생성
    // 주문 관리 객체에서는 고객, 제품 검색 다이얼로그를 사용한다.
    orderForm = new OrderManagerForm(this, clientDialog, productDialog);
    connect(orderForm, SIGNAL(destroyed()),
            orderForm, SLOT(deleteLater()));
    orderForm->setWindowTitle(tr("Order Info"));

    // 채팅 서버 객체 생성
    chatForm = new ChatServerForm(this);
    connect(chatForm, SIGNAL(destroyed()),
            chatForm, SLOT(deleteLater()));
    chatForm->setWindowTitle(tr("Chat server"));

    /* 객체 간 signal과 slot을 connect */
    // 고객 검색 다이얼로그에서 검색어를 고객 관리 객체로 전달해줌
    connect(clientDialog, SIGNAL(sendWord(QString)), \
            clientForm, SLOT(receiveWord(QString)));
    // 고객 관리 객체에서 검색 결과를 고객 검색 다이얼로그로 전달해줌
    connect(clientForm, SIGNAL(sendClientToDialog(int,QString,QString,QString)), \
            clientDialog, SLOT(receiveClientInfo(int,QString,QString,QString)));

    // 제품 검색 다이얼로그에서 검색어를 제품 관리 객체로 전달해줌
    connect(productDialog, SIGNAL(sendWord(QString)), \
            productForm, SLOT(receiveWord(QString)));
    // 제품 관리 객체에서 검색 결과를 제품 검색 다이얼로그로 전달해줌
    connect(productForm, SIGNAL(sendProductToDialog(int,QString,QString,int,int)), \
            productDialog, SLOT(receiveProductInfo(int,QString,QString,int,int)));

    // 주문 관리 객체에서 정보를 받아올 고객의 id를 고객 관리 객체로 전달해줌
    connect(orderForm, SIGNAL(sendClientId(int)), \
            clientForm, SLOT(receiveId(int)));
    // 고객 관리 객체에서 고객의 정보를 주문 관리 객체로 전달해줌
    connect(clientForm, SIGNAL(sendClientToOrderManager(int,QString,QString,QString)), \
            orderForm, SLOT(receiveClientInfo(int,QString,QString,QString)));
    // 주문 관리 객체에서 정보를 받아올 제품의 id를 제품 관리 객체로 전달해줌
    connect(orderForm, SIGNAL(sendProductId(int)), \
            productForm, SLOT(receiveId(int)));
    // 제품 관리 객체에서 제품의 정보를 주문 관리 객체로 전달해줌
    connect(productForm, SIGNAL(sendProductToManager(int,QString,QString,int,int)), \
            orderForm, SLOT(receiveProductInfo(int,QString,QString,int,int)));

    // 고객 관리 객체에서 고객의 정보를 채팅 서버 객체로 전달해줌
    connect(clientForm, SIGNAL(sendClientToChatServer(int,QString)), \
            chatForm, SLOT(addClient(int,QString)));

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

    connect(clientForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int)));
    connect(productForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int)));
    connect(orderForm, SIGNAL(sendStatusMessage(QString,int)), \
            this->statusBar(), SLOT(showMessage(const QString &,int)));

    connect(orderForm, SIGNAL(sendStock(int,int)), \
            productForm, SLOT(setStock(int,int)));
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


void MainWindow::on_actionClient_triggered()
{
    if(clientForm != nullptr) {
        clientForm->setFocus();
    }
}

void MainWindow::on_actionProduct_triggered()
{
    if(productForm != nullptr) {
        productForm->setFocus();
    }
}

void MainWindow::on_actionOrder_triggered()
{
    if(orderForm != nullptr) {
        orderForm->setFocus();
    }
}

void MainWindow::on_actionChat_triggered()
{
    if(chatForm != nullptr) {
        chatForm->setFocus();
    }
}


