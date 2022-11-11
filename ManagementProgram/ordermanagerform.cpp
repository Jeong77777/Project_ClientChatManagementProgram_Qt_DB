#include "ordermanagerform.h"
#include "ui_ordermanagerform.h"
#include "clientdialog.h"
#include "productdialog.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
//#include <QTreeWidgetItem>

/**
* @brief 생성자, split 사이즈 설정, 입력 칸 초기화, context 메뉴 설정, 검색 관련 초기 설정
*/
OrderManagerForm::OrderManagerForm(QWidget *parent, \
                                   ClientDialog *clientDialog, \
                                   ProductDialog *productDialog) :
    QWidget(parent), clientDialog(clientDialog), productDialog(productDialog),
    ui(new Ui::OrderManagerForm)
{
    ui->setupUi(this);
    ui->searchDateEdit->setDate(QDate::currentDate());

    /* 검색 결과를 저장하는 flag 초기화 */
    searchedClientFlag = false;
    searchedProductFlag = false;

    /* 입력 창 초기화 */
    cleanInputLineEdit();

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 170 << 400;
    ui->splitter->setSizes(sizes);

    /* tree widget의 context 메뉴 설정 */
    QAction* removeAction = new QAction(tr("Remove"));
    connect(removeAction, SIGNAL(triggered()), SLOT(removeItem()));
    menu = new QMenu; // context 메뉴
    menu->addAction(removeAction);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), \
            this, SLOT(showContextMenu(QPoint)));

    /* 검색 창에서 enter 키를 누르면 검색 버튼이 클릭되도록 connect */
    connect(ui->searchLineEdit, SIGNAL(returnPressed()), \
            this, SLOT(on_searchPushButton_clicked()));

    /* 검색에서 date를 선택하는 date edit를 숨김 */
    // 실행 시 초기에는 검색할 항목이 date로 지정되어 있지 않으므로 숨긴다.
    ui->searchDateEdit->hide();
}

/**
* @brief orderlist.txt 파일을 열어서 저장된 주문 리스트를 가져옴
*/
void OrderManagerForm::loadData()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "orderConnection");
    db.setDatabaseName("orderlist.db");
    if (db.open( )) {
        QSqlQuery query(db);
        query.exec( "CREATE TABLE IF NOT EXISTS Order_list ("
                    "id          INTEGER          PRIMARY KEY, "
                    "date        DATE             NOT NULL,"
                    "client      VARCHAR(50)      NOT NULL,"
                    "product     VARCHAR(50)      NOT NULL,"
                    "quantity    INTEGER          NOT NULL,"
                    "total       INTEGER          NOT NULL"
                    " )"
                    );

        orderModel = new QSqlTableModel(this, db);
        orderModel->setTable("Order_list");
        orderModel->select();
        orderModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
        orderModel->setHeaderData(1, Qt::Horizontal, tr("Date"));
        orderModel->setHeaderData(2, Qt::Horizontal, tr("Client"));
        orderModel->setHeaderData(3, Qt::Horizontal, tr("Product"));
        orderModel->setHeaderData(4, Qt::Horizontal, tr("Quantity"));
        orderModel->setHeaderData(5, Qt::Horizontal, tr("Total"));
        ui->treeView->setModel(orderModel);
    }
}

/**
* @brief 소멸자, 주문 리스트를 orderlist.txt에 저장
*/
OrderManagerForm::~OrderManagerForm()
{
    delete ui;
    QSqlDatabase db = QSqlDatabase::database("orderConnection");
    if(db.isOpen()) {
        orderModel->submitAll();
        delete orderModel;
        db.commit();
        db.close();
    }
}


/**
* @brief 전체 주문 리스트 출력 버튼 슬롯, tree widget에 전체 주문 리스트를 출력해 준다.
*/
void OrderManagerForm::on_showAllPushButton_clicked()
{
    orderModel->setFilter("");
    orderModel->select();
    ui->searchLineEdit->clear(); // 검색 창 클리어
}

/**
* @brief 검색 항목 선택 콤보 박스에서 선택된 것이 변경되었을 때 실행되는 슬롯
*/
void OrderManagerForm::on_searchComboBox_currentIndexChanged(int index)
{
    // 0. ID  1. Date  2. Client  3. Product
    if(index == 1) { // 검색 항목으로 Date를 선택 했을 때
        ui->searchLineEdit->hide(); // line edit를 숨겨준다.
        ui->searchDateEdit->show(); // date edit를 보여준다.
    }
    else { // 검색 항목으로 ID, Client, Product를 선택 했을 때
        ui->searchDateEdit->hide(); // date edit를 숨겨준다.
        ui->searchLineEdit->show(); // line edit를 보여준다.
    }
}

/**
* @brief 검색 버튼 슬롯, tree widget에 검색 결과를 출력해 준다.
*/
void OrderManagerForm::on_searchPushButton_clicked()
{
    /* 현재 선택된 검색 항목 */
    // 0. ID  1. Date  2. Client  3. Product
    int i = ui->searchComboBox->currentIndex();

    /* 검색 수행 */
    QString str; // 검색어
    if(i != 1) { // ID, Client, Product
        str = ui->searchLineEdit->text();
        if(!str.length()) { // 검색 창이 비어 있을 때
            QMessageBox::warning(this, tr("Search error"), \
                                     tr("Please enter a search term."), \
                                     QMessageBox::Ok);
            return;
        }
    }
    else         // Date
        str = ui->searchDateEdit->date().toString("yyyy-MM-dd");

    switch (i) {
    case 0: orderModel->setFilter(QString("id = '%1'").arg(str));
        break;
    case 1: orderModel->setFilter(QString("date = '%1'").arg(str));
        break;
    case 2: orderModel->setFilter(QString("client LIKE '%%1%'").arg(str));
        break;
    case 3: orderModel->setFilter(QString("product LIKE '%%1%'").arg(str));
        break;
    default:
        break;
    }
    orderModel->select();
    emit sendStatusMessage(tr("%1 search results were found").arg(orderModel->rowCount()), 3000);

    QString filterStr = "id in (";
    for(int i = 0; i < orderModel->rowCount(); i++) {
        int id = orderModel->data(orderModel->index(i, 0)).toInt();
        if(i != orderModel->rowCount()-1)
            filterStr += QString("%1, ").arg(id);
        else
            filterStr += QString("%1").arg(id);
    }
    filterStr += ");";
    qDebug() << filterStr;
    orderModel->setFilter(filterStr);

    ui->clientTreeWidget->clear();  // 고객 상세 정보 클리어
    ui->productTreeWidget->clear(); // 제품 상세 정보 클리어
}

/**
* @brief 고객 검색, 입력 버튼 슬롯 -> 다이얼로그 실행
*/
void OrderManagerForm::on_inputClientPushButton_clicked()
{
    /* 고객을 검색, 입력하기 위한 다이얼로그 실행 */
    clientDialog->show();

    if (clientDialog->exec() == QDialog::Accepted) { // OK 버튼을 누르면
        // 다이얼로그에서 선택한 고객을 가져온다.
        QTreeWidgetItem* c = clientDialog->getCurrentItem();
        if(c!=nullptr) {
            // 주문 정보 입력 칸에 고객 ID와 이름을 입력한다.
            ui->clientLineEdit->setText(c->text(0) + " (" + c->text(1) + ")");
        }
    }
    clientDialog->clearDialog(); // 다이얼로그 초기화
}

/**
* @brief 제품 검색, 입력 버튼 슬롯 -> 다이얼로그 실행
*/
void OrderManagerForm::on_inputProductPushButton_clicked()
{
    /* 제품을 검색, 입력하기 위한 다이얼로그 실행 */
    productDialog->show();

    if (productDialog->exec() == QDialog::Accepted) { // OK 버튼을 누르면
        // 다이얼로그에서 선택한 제품을 가져온다.
        QTreeWidgetItem* p = productDialog->getCurrentItem();
        if(p!=nullptr) {
            // 주문 정보 입력 칸에 제품 ID와 이름을 입력한다.
            ui->productLineEdit->setText(p->text(0) + " (" + p->text(2) + ")");
        }
    }
    productDialog->clearDialog(); // 다이얼로그 초기화
}

/**
* @brief 주문 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문을 추가함
*/
void OrderManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    QString date, clientName, productName, total;
    int id = makeId(); // 자동으로 ID 생성
    int clientId, productId, quantity;
    clientId = ui->clientLineEdit->text().split(" ")[0].toInt();
    productId = ui->productLineEdit->text().split(" ")[0].toInt();
    quantity = ui->quantitySpinBox->text().toInt();
    clientName = ui->clientLineEdit->text();
    productName = ui->productLineEdit->text();
    date = ui->dateEdit->date().toString("yyyy-MM-dd");

    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
    emit sendClientId(clientId);
    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
    emit sendProductId(productId);

    /* 입력된 정보를 DB에 추가 */
    QSqlDatabase db = QSqlDatabase::database("orderConnection");
    if(db.isOpen()) {
        try {
            if(searchedClientFlag == false)                // 고객 정보가 존재하지 않을 때
                throw tr("Customer information does not exist.");
            else if(searchedProductFlag == false)          // 제품 정보가 존재하지 않을 때
                throw tr("Product information does not exist.");
            else if(quantity == 0)                         // 올바른 수량을 입력하지 않았을 때
                throw tr("Please enter a valid quantity.");
            else if(searchedProduct->text(4).toInt() < quantity) // 재고가 부족할 때
                throw tr("There is a shortage of stock.");

            // 주문 금액 계산
            total = QString::number(quantity * searchedProduct->text(3).toInt());

            QSqlQuery query(orderModel->database());
            query.prepare( "INSERT INTO Order_list "
                           "(id, date, client, product, quantity, total) "
                           "VALUES "
                           "(:ID, :DATE, :CLIENT, :PRODUCT, :QUANTITY, :TOTAL)" );
            query.bindValue(":ID",        id);
            query.bindValue(":DATE",      date);
            query.bindValue(":CLIENT",    clientName);
            query.bindValue(":PRODUCT",   productName);
            query.bindValue(":QUANTITY",  quantity);
            query.bindValue(":TOTAL",     total);
            query.exec();
            orderModel->select();

            // 주문 수량만큼 제품의 재고 차감
            emit sendStock(productId, searchedProduct->text(4).toInt() - quantity);

            cleanInputLineEdit(); // 입력 창 클리어

            emit sendStatusMessage(tr("Modify completed (ID: %1, Client: %2, Product: %3)").arg(id).arg(clientName).arg(productName), 3000);

        } catch (QString msg) { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Add error"),
                                 QString(msg), QMessageBox::Ok);
        }

        searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
        searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
    }
}

/**
* @brief 주문 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문 정보를 변경함
*/
void OrderManagerForm::on_modifyPushButton_clicked()
{
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = orderModel->data(index.siblingAtColumn(0)).toInt();
        // 입력 창에 입력된 정보 가져오기
        QString date, clientName, productName, total;
        int clientId, productId, oldQuantity, newQuantity;
        clientId = ui->clientLineEdit->text().split(" ")[0].toInt();
        productId = ui->productLineEdit->text().split(" ")[0].toInt();
        oldQuantity = orderModel->data(index.siblingAtColumn(4)).toInt();       // 변경 전 주문 수량
        newQuantity = ui->quantitySpinBox->text().toInt(); // 변경 후 주문 수량
        clientName = ui->clientLineEdit->text();
        productName = ui->productLineEdit->text();
        date = ui->dateEdit->date().toString("yyyy-MM-dd");

        // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
        emit sendClientId(clientId);
        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
        emit sendProductId(productId);

        /* 입력 창에 입력된 정보에 따라 주문 정보를 변경 */
        try {
            if(searchedClientFlag == false)       // 고객 정보가 존재하지 않을 때
                throw tr("Customer information does not exist.");
            else if(searchedProductFlag == false) // 제품 정보가 존재하지 않을 때
                throw tr("Product information does not exist.");
            else if(newQuantity == 0)             // 올바른 수량을 입력하지 않았을 때
                throw tr("Please enter a valid quantity.");
            else if(searchedProduct->text(4).toInt() + oldQuantity \
                    < newQuantity)                // 재고가 부족할 때
                throw tr("There is a shortage of stock.\n") + tr("Maximum: ")
                    + QString::number(searchedProduct->text(4).toInt() + oldQuantity);

            // 주문 금액 계산
            total = QString::number(newQuantity * searchedProduct->text(3).toInt());

            // 주문 수량만큼 제품의 재고 차감
            emit sendStock(productId, searchedProduct->text(4).toInt() + oldQuantity - newQuantity);

            // 입력 창에 입력된 정보에 따라 주문 정보를 변경
            QSqlQuery query(orderModel->database());
            query.prepare("UPDATE Order_list SET date = ?, client = ?, product = ?, quantity = ? , total = ? WHERE id = ?");
            query.bindValue(0, date);
            query.bindValue(1, clientName);
            query.bindValue(2, productName);
            query.bindValue(3, newQuantity);
            query.bindValue(4, total);
            query.bindValue(5, id);
            query.exec();
            orderModel->select();

            emit sendStatusMessage(tr("Modify completed (ID: %1, Client: %2, Product: %3)").arg(id).arg(clientName).arg(productName), 3000);

            searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
            searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경

            // 주문 정보가 변경 됨에 따라
            // 고객, 제품 상세 정보 tree widget에 새로운 정보 표시를 위해서 다음의 함수 호출
            on_treeView_clicked(index);

            cleanInputLineEdit(); // 입력 창 클리어
        }
        catch (QString msg) { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Add error"),
                                 QString(msg), QMessageBox::Ok);
        }

        searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
        searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
    }
}

/**
* @brief 입력 창 클리어 버튼 슬롯, 입력 창을 클리어 하는 함수 호출
*/
void OrderManagerForm::on_cleanPushButton_clicked()
{
    cleanInputLineEdit();
}

/**
* @brief tree view의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void OrderManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree view 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeView->mapToGlobal(pos);
    if(ui->treeView->indexAt(pos).isValid())
        menu->exec(globalPos);
}

/**
* @brief 고객 정보 삭제
*/
void OrderManagerForm::removeItem()
{
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        // 삭제된 재고를 다시 추가하기 위해서
        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
        int productId = orderModel->data(index.siblingAtColumn(3)).toString().split(" ")[0].toInt();
        emit sendProductId(productId);
        if(searchedProductFlag == true) { // 제품 정보가 존재할 때
            // OK를 누르면 삭제된 재고를 다시 추가
            if(QMessageBox::Yes == QMessageBox::information(this, tr("Order list remove"), \
                                                            tr("Do you want to re-add the deleted inventory?"), \
                                                            QMessageBox::Yes|QMessageBox::No))
                emit sendStock(productId, searchedProduct->text(4).toInt() + orderModel->data(index.siblingAtColumn(4)).toInt());

        }
        searchedProductFlag = false; // 고객 검색 결과 flag를 다시 false로 변경
        int id = orderModel->data(index.siblingAtColumn(0)).toInt();
        QString client = orderModel->data(index.siblingAtColumn(2)).toString();
        QString product = orderModel->data(index.siblingAtColumn(2)).toString();
        orderModel->removeRow(index.row());
        orderModel->select();

        ui->clientTreeWidget->clear();  // 고객 상세 정보 클리어
        ui->productTreeWidget->clear(); // 제품 상세 정보 클리어

        emit sendStatusMessage(tr("delete completed (ID: %1, Client: %2, Product: %3)").arg(id).arg(client).arg(product), 3000);
    }
}

/**
* @brief 고객 정보 관리 객체로부터 고객 정보를 받기 위한 슬롯
* @Param QTreeWidgetItem* c 가져온 고객
*/
void OrderManagerForm::receiveClientInfo(QTreeWidgetItem* c)
{
    searchedClient = c;        // 고객을 저장
    searchedClientFlag = true; // 고객 검색 결과를 true로 변경
}

/**
* @brief 제품 정보 관리 객체로부터 제품 정보를 받기 위한 슬롯
* @Param ProductItem* p 가져온 제품
*/
void OrderManagerForm::receiveProductInfo(QTreeWidgetItem* p)
{
    searchedProduct = p;        // 제품을 저장
    searchedProductFlag = true; // 제품 검색 결과를 true로 변경
}

/**
* @brief 신규 주문 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int OrderManagerForm::makeId()
{
    QSqlQuery query("select count(*), max(id) from Order_list;",
                    orderModel->database());
    query.exec();
    while (query.next()) {
        if(query.value(0).toInt() == 0)
            return 1000001; // id는 1000001부터 시작
        else {
            auto id = query.value(1).toInt();
            return ++id; // 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
        }
    }
    return 1;
}

/**
* @brief 입력 창 클리어
*/
void OrderManagerForm::cleanInputLineEdit()
{
    ui->idLineEdit->clear();
    ui->dateEdit->setDate(QDate::currentDate());
    ui->clientLineEdit->clear();
    ui->productLineEdit->clear();
    ui->quantitySpinBox->clear();
}

void OrderManagerForm::on_treeView_clicked(const QModelIndex &index)
{
    QString id = orderModel->data(index.siblingAtColumn(0)).toString();
    QString date = orderModel->data(index.siblingAtColumn(1)).toString();
    QString client = orderModel->data(index.siblingAtColumn(2)).toString();
    QString product = orderModel->data(index.siblingAtColumn(3)).toString();
    int quantity = orderModel->data(index.siblingAtColumn(4)).toInt();

    /* 클릭된 제품의 정보를 입력 창에 표시해줌 */
    ui->idLineEdit->setText(id);
    ui->dateEdit->setDate(QDate::fromString(date, "yyyy-MM-dd"));
    ui->clientLineEdit->setText(client);
    ui->productLineEdit->setText(product);
    ui->quantitySpinBox->setValue(quantity);

    searchedClientFlag = false;  // 고객 검색 결과 flag 초기화
    searchedProductFlag = false; // 제품 검색 결과 flag 초기화

    /* 고객, 제품 상세 정보 tree widget에 상세 정보 표시 */

    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
    int clientId = client.split(" ")[0].toInt();
    emit sendClientId(clientId);
    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
    int productId = product.split(" ")[0].toInt();
    emit sendProductId(productId);

    ui->clientTreeWidget->clear();  // 고객 상세 정보 클리어
    ui->productTreeWidget->clear(); // 제품 상세 정보 클리어

    if(searchedClientFlag == true) { // 고객 정보가 존재할 때
        // 고객 상세 정보 표시
        ui->clientTreeWidget->addTopLevelItem(searchedClient);
    }
    if(searchedProductFlag == true) { // 제품 정보가 존재할 때
        // 제품 상세 정보 표시
        ui->productTreeWidget->addTopLevelItem(searchedProduct);
    }

    searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
    searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
}
