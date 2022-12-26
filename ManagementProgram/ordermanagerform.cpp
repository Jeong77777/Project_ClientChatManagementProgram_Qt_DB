#include "ordermanagerform.h"
#include "ui_ordermanagerform.h"
#include "clientdialog.h"
#include "productdialog.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QStandardItemModel>
#include <QMessageBox>
#include <cassert>
#include <vector>

/**
* @brief 생성자, split 사이즈 설정, 입력 칸 초기화, context 메뉴 설정, 검색 관련 초기 설정
*/
OrderManagerForm::OrderManagerForm(QWidget *parent, \
                                   ClientDialog *clientDialog, \
                                   ProductDialog *productDialog) :
    QWidget(parent), m_clientDialog(clientDialog), m_productDialog(productDialog),
    m_ui(new Ui::OrderManagerForm), m_menu(nullptr), m_orderModel(nullptr),
    m_clientModel(nullptr), m_productModel(nullptr),
    m_searchedClientFlag(false), m_searchedProductFlag(false)
{
    m_ui->setupUi(this);
    m_ui->searchDateEdit->setDate(QDate::currentDate());

    /* 입력 창 초기화 */
    cleanInputLineEdit();

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 170 << 400;
    m_ui->splitter->setSizes(sizes);

    /* tree widget의 context 메뉴 설정 */
    m_removeAction = new QAction(tr("Remove"));
    assert(connect(m_removeAction, SIGNAL(triggered()), SLOT(removeItem())));
    m_menu = new QMenu; // context 메뉴
    m_menu->addAction(m_removeAction);
    m_ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    assert(connect(m_ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), \
            this, SLOT(showContextMenu(QPoint))));

    /* 검색 창에서 enter 키를 누르면 검색 버튼이 클릭되도록 connect */
    assert(connect(m_ui->searchLineEdit, SIGNAL(returnPressed()), \
            this, SLOT(on_searchPushButton_clicked())));

    /* 검색에서 date를 선택하는 date edit를 숨김 */
    // 실행 시 초기에는 검색할 항목이 date로 지정되어 있지 않으므로 숨긴다.
    m_ui->searchDateEdit->hide();

    /* client model 초기화 */
    m_clientModel = new QStandardItemModel(0, 4);
    m_clientModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    m_clientModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    m_clientModel->setHeaderData(2, Qt::Horizontal, tr("Phone Number"));
    m_clientModel->setHeaderData(3, Qt::Horizontal, tr("Address"));
    m_ui->clientTreeView->setModel(m_clientModel);

    /* product model 초기화 */
    m_productModel = new QStandardItemModel(0, 5);
    m_productModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    m_productModel->setHeaderData(1, Qt::Horizontal, tr("Type"));
    m_productModel->setHeaderData(2, Qt::Horizontal, tr("Name"));
    m_productModel->setHeaderData(3, Qt::Horizontal, tr("Unit Price"));
    m_productModel->setHeaderData(4, Qt::Horizontal, tr("Quantities in stock"));
    m_ui->productTreeView->setModel(m_productModel);
}

/**
* @brief 주문 정보 데이터베이스 open
*/
void OrderManagerForm::loadData()
{
    /* 주문 정보 데이터베이스 open */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "orderConnection");
    db.setDatabaseName("orderlist.db");
    if (db.open( )) {
        // 주문 정보 테이블 생성
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

        // model로 데이터 베이스를 가져옴
        m_orderModel = new QSqlTableModel(this, db);
        m_orderModel->setTable("Order_list");
        m_orderModel->select();
        m_orderModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
        m_orderModel->setHeaderData(1, Qt::Horizontal, tr("Date"));
        m_orderModel->setHeaderData(2, Qt::Horizontal, tr("Client"));
        m_orderModel->setHeaderData(3, Qt::Horizontal, tr("Product"));
        m_orderModel->setHeaderData(4, Qt::Horizontal, tr("Quantity"));
        m_orderModel->setHeaderData(5, Qt::Horizontal, tr("Total"));
        m_ui->treeView->setModel(m_orderModel);
    }
}

/**
* @brief 소멸자, 주문 정보 데이터베이스 저장하고 닫기
*/
OrderManagerForm::~OrderManagerForm()
{
    QSqlDatabase db = QSqlDatabase::database("orderConnection");
    if(db.isOpen()) {
        m_orderModel->submitAll();
        delete m_orderModel; m_orderModel = nullptr;
        db.commit();
        db.close();
    }

    delete m_removeAction; m_removeAction = nullptr;
    delete m_menu; m_menu = nullptr;
    delete m_clientModel; m_clientModel = nullptr;
    delete m_productModel; m_productModel = nullptr;
    delete m_ui; m_ui = nullptr;
}


/**
* @brief 전체 주문 리스트 출력 버튼 슬롯, tree view에 전체 주문 리스트를 출력해 준다.
*/
void OrderManagerForm::on_showAllPushButton_clicked() const
{
    m_orderModel->setFilter("");  // 필터 초기화
    m_orderModel->select();
    m_ui->searchLineEdit->clear(); // 검색 창 클리어

    // 고객, 제품 상세 정보 초기화
    m_clientModel->removeRows(0, m_clientModel->rowCount());
    m_productModel->removeRows(0, m_productModel->rowCount());
}

/**
* @brief 검색 항목 선택 콤보 박스에서 선택된 것이 변경되었을 때 실행되는 슬롯
* @Param int index 선택된 항목의 index
*/
void OrderManagerForm::on_searchComboBox_currentIndexChanged(const int index) const
{
    // 0. ID  1. Date  2. Client  3. Product
    if(index == 1) { // 검색 항목으로 Date를 선택 했을 때
        m_ui->searchLineEdit->hide(); // line edit를 숨겨준다.
        m_ui->searchDateEdit->show(); // date edit를 보여준다.
    }
    else { // 검색 항목으로 ID, Client, Product를 선택 했을 때
        m_ui->searchDateEdit->hide(); // date edit를 숨겨준다.
        m_ui->searchLineEdit->show(); // line edit를 보여준다.
    }
}

/**
* @brief 검색 버튼 슬롯, tree view에 검색 결과를 출력해 준다.
*/
void OrderManagerForm::on_searchPushButton_clicked()
{
    /* 현재 선택된 검색 항목 */
    // 0. ID  1. Date  2. Client  3. Product
    int i = m_ui->searchComboBox->currentIndex();

    /* 검색 수행 */
    std::string str; // 검색어

    if(i != 1) { // ID, Client, Product
        str = m_ui->searchLineEdit->text().toStdString();
        if(!str.length()) { // 검색 창이 비어 있을 때
            QMessageBox::warning(this, tr("Search error"), \
                                 tr("Please enter a search term."), \
                                 QMessageBox::Ok);
            return;
        }
    }
    else         // Date
        str = m_ui->searchDateEdit->date().toString("yyyy-MM-dd").toStdString();

    // 0. ID  1. Date  2. Client  3. Product
    switch (i) {
    case 0: m_orderModel->setFilter(QString("id = '%1'").arg(QString::fromStdString(str)));
        break;
    case 1: m_orderModel->setFilter(QString("date = '%1'").arg(QString::fromStdString(str)));
        break;
    case 2: m_orderModel->setFilter(QString("client LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    case 3: m_orderModel->setFilter(QString("product LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    default:
        break;
    }
    m_orderModel->select();

    // status bar 메시지 출력
    emit sendStatusMessage(tr("%1 search results were found") \
                           .arg(m_orderModel->rowCount()), 3000);

    /* 사용자가 정보를 변경해도 검색 결과가 유지되도록 ID를 이용해서 필터 재설정 */
    std::string filterStr = "id in (";
    for(int i = 0; i < m_orderModel->rowCount(); i++) {
        int id = m_orderModel->data(m_orderModel->index(i, 0)).toInt();
        if(i != m_orderModel->rowCount()-1)
            filterStr += (std::to_string(id) + ", ");
        else
            filterStr += std::to_string(id);
    }
    filterStr += ");";
    qDebug() << QString::fromStdString(filterStr);
    m_orderModel->setFilter(QString::fromStdString(filterStr));

    m_clientModel->removeRows(0, m_clientModel->rowCount());
    m_productModel->removeRows(0, m_productModel->rowCount());
}

/**
* @brief 고객 검색, 입력 버튼 슬롯 -> 다이얼로그 실행
*/
void OrderManagerForm::on_inputClientPushButton_clicked() const
{
    /* 고객을 검색, 입력하기 위한 다이얼로그 실행 */
    m_clientDialog->show();

    if (m_clientDialog->exec() == QDialog::Accepted) { // OK 버튼을 누르면
        // 다이얼로그에서 선택한 고객을 가져온다.
        m_ui->clientLineEdit->setText(\
                    QString::fromStdString(m_clientDialog->getCurrentItem()));
    }
    m_clientDialog->clearDialog(); // 다이얼로그 초기화
}

/**
* @brief 제품 검색, 입력 버튼 슬롯 -> 다이얼로그 실행
*/
void OrderManagerForm::on_inputProductPushButton_clicked() const
{
    /* 제품을 검색, 입력하기 위한 다이얼로그 실행 */
    m_productDialog->show();

    if (m_productDialog->exec() == QDialog::Accepted) { // OK 버튼을 누르면
        // 다이얼로그에서 선택한 제품을 가져온다.
        m_ui->productLineEdit->setText(\
                    QString::fromStdString(m_productDialog->getCurrentItem()));
    }
    m_productDialog->clearDialog(); // 다이얼로그 초기화
}

/**
* @brief 주문 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문을 추가함
*/
void OrderManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    std::string date, clientName, productName, total;
    int id = makeId(); // 자동으로 ID 생성
    int clientId, productId, quantity;
    clientId = m_ui->clientLineEdit->text().split(" ")[0].toInt();
    productId = m_ui->productLineEdit->text().split(" ")[0].toInt();
    quantity = m_ui->quantitySpinBox->text().toInt();
    clientName = m_ui->clientLineEdit->text().toStdString();
    productName = m_ui->productLineEdit->text().toStdString();
    date = m_ui->dateEdit->date().toString("yyyy-MM-dd").toStdString();

    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
    emit sendClientId(clientId);
    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
    emit sendProductId(productId);

    /* 입력된 정보를 DB에 추가 */
    QSqlDatabase db = QSqlDatabase::database("orderConnection");
    if(db.isOpen()) {
        try {
            if(m_searchedClientFlag == false)                // 고객 정보가 존재하지 않을 때
                throw tr("Customer information does not exist.");
            else if(m_searchedProductFlag == false)          // 제품 정보가 존재하지 않을 때
                throw tr("Product information does not exist.");
            else if(quantity == 0)                         // 올바른 수량을 입력하지 않았을 때
                throw tr("Please enter a valid quantity.");
            else if(m_productModel->data(m_productModel->index(0,4)).toInt() < quantity) // 재고가 부족할 때
                throw tr("There is a shortage of stock.");

            // 주문 금액 계산
            total = std::to_string(quantity \
                                    * m_productModel->data(m_productModel->index(0,3)).toInt());

            QSqlQuery query(m_orderModel->database());
            query.prepare( "INSERT INTO Order_list "
                           "(id, date, client, product, quantity, total) "
                           "VALUES "
                           "(:ID, :DATE, :CLIENT, :PRODUCT, :QUANTITY, :TOTAL)" );
            query.bindValue(":ID",        id);
            query.bindValue(":DATE",      QString::fromStdString(date));
            query.bindValue(":CLIENT",    QString::fromStdString(clientName));
            query.bindValue(":PRODUCT",   QString::fromStdString(productName));
            query.bindValue(":QUANTITY",  quantity);
            query.bindValue(":TOTAL",     QString::fromStdString(total));
            query.exec();
            m_orderModel->select();

            // 주문 수량만큼 제품의 재고 차감
            emit sendStock(productId, \
                           m_productModel->data(m_productModel->index(0,4)).toInt() - quantity);

            cleanInputLineEdit(); // 입력 창 클리어

            emit sendStatusMessage(tr("Add completed (ID: %1, Client: %2, Product: %3)") \
                                   .arg(id).arg(QString::fromStdString(clientName))\
                                   .arg(QString::fromStdString(productName)), 3000);

        } catch (QString msg) { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Add error"),
                                 QString(msg), QMessageBox::Ok);
        }

        m_searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
        m_searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경

        // 고객, 제품 상세 정보 초기화
        m_clientModel->removeRows(0, m_clientModel->rowCount());
        m_productModel->removeRows(0, m_productModel->rowCount());
    }
}

/**
* @brief 주문 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문 정보를 변경함
*/
void OrderManagerForm::on_modifyPushButton_clicked()
{
    /* tree view에서 현재 선택된 고객의 index 가져오기 */
    QModelIndex index = m_ui->treeView->currentIndex();

    /* 입력 창에 입력된 정보에 따라 주문 정보를 변경 */
    if(index.isValid()) {
        // 입력 창에 입력된 정보 가져오기
        int id = m_orderModel->data(index.siblingAtColumn(0)).toInt();
        std::string date, clientName, productName, total;
        int clientId, productId, oldQuantity, newQuantity;
        clientId = m_ui->clientLineEdit->text().split(" ")[0].toInt();
        productId = m_ui->productLineEdit->text().split(" ")[0].toInt();
        // 변경 전 주문 수량
        oldQuantity = m_orderModel->data(index.siblingAtColumn(4)).toInt();
        // 변경 후 주문 수량
        newQuantity = m_ui->quantitySpinBox->text().toInt();
        clientName = m_ui->clientLineEdit->text().toStdString();
        productName = m_ui->productLineEdit->text().toStdString();
        date = m_ui->dateEdit->date().toString("yyyy-MM-dd").toStdString();

        // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
        emit sendClientId(clientId);
        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
        emit sendProductId(productId);

        /* 입력 창에 입력된 정보에 따라 주문 정보를 변경 */
        try {
            if(m_searchedClientFlag == false)       // 고객 정보가 존재하지 않을 때
                throw tr("Customer information does not exist.");
            else if(m_searchedProductFlag == false) // 제품 정보가 존재하지 않을 때
                throw tr("Product information does not exist.");
            else if(newQuantity == 0)             // 올바른 수량을 입력하지 않았을 때
                throw tr("Please enter a valid quantity.");
            else if(m_productModel->data(m_productModel->index(0,4)).toInt() + oldQuantity \
                    < newQuantity)                // 재고가 부족할 때
                throw tr("There is a shortage of stock.\n") + tr("Maximum: ")
                    + QString::number(m_productModel->data(m_productModel->index(0,4)).toInt() \
                                      + oldQuantity);

            // 총 주문 금액 계산
            total = std::to_string(newQuantity \
                                    * m_productModel->data(m_productModel->index(0,3)).toInt());

            // 주문 수량만큼 제품의 재고 차감
            emit sendStock(productId, \
                           m_productModel->data(m_productModel->index(0,4)).toInt() \
                           + oldQuantity - newQuantity);

            // 입력 창에 입력된 정보에 따라 주문 정보를 변경
            QSqlQuery query(m_orderModel->database());
            query.prepare("UPDATE Order_list "
                          "SET date = ?, client = ?, product = ?, "
                          "quantity = ?, total = ? WHERE id = ?");
            query.bindValue(0, QString::fromStdString(date));
            query.bindValue(1, QString::fromStdString(clientName));
            query.bindValue(2, QString::fromStdString(productName));
            query.bindValue(3, newQuantity);
            query.bindValue(4, QString::fromStdString(total));
            query.bindValue(5, id);
            query.exec();
            m_orderModel->select();

            // status bar 메시지 출력
            emit sendStatusMessage(tr("Modify completed (ID: %1, Client: %2, Product: %3)") \
                                   .arg(id).arg(QString::fromStdString(clientName))\
                                   .arg(QString::fromStdString(productName)), 3000);

            m_searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
            m_searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경

            // 주문 정보가 변경 됨에 따라
            // 고객, 제품 상세 정보 tree view에 새로운 정보 표시를 위해서 다음의 함수 호출
            on_treeView_clicked(index);

            cleanInputLineEdit(); // 입력 창 클리어
        }
        catch (QString msg) { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Add error"),
                                 QString(msg), QMessageBox::Ok);
        }

        m_searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
        m_searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
    }
}

/**
* @brief 입력 창 클리어 버튼 슬롯, 입력 창을 클리어 하는 함수 호출
*/
void OrderManagerForm::on_cleanPushButton_clicked() const
{
    cleanInputLineEdit();
}

/**
* @brief tree view의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void OrderManagerForm::showContextMenu(const QPoint &pos) const
{
    /* tree view 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = m_ui->treeView->mapToGlobal(pos);
    if(m_ui->treeView->indexAt(pos).isValid())
        m_menu->exec(globalPos);
}

/**
* @brief 주문 정보 삭제
*/
void OrderManagerForm::removeItem()
{
    /* tree view에서 현재 선택된 주문의 index 가져오기 */
    QModelIndex index = m_ui->treeView->currentIndex();

    /* 주문 정보 삭제 */
    if(index.isValid()) {
        // 삭제된 재고를 다시 추가하기 위해서
        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
        int productId = m_orderModel->data(index.siblingAtColumn(3)) \
                .toString().split(" ")[0].toInt();
        emit sendProductId(productId);
        if(m_searchedProductFlag == true) { // 제품 정보가 존재할 때
            // OK를 누르면 삭제된 재고를 다시 추가
            if(QMessageBox::Yes \
                    == QMessageBox::information(this, tr("Order list remove"), \
                                                tr("Do you want to re-add the deleted inventory?"), \
                                                QMessageBox::Yes|QMessageBox::No))

                // 제품 정보 관리 객체를 통해서 재고 수량 변경
                emit sendStock(productId, m_productModel->data(m_productModel->index(0,4)).toInt() \
                               + m_orderModel->data(index.siblingAtColumn(4)).toInt());

        }
        m_searchedProductFlag = false; // 고객 검색 결과 flag를 다시 false로 변경

        // 주문 정보 삭제
        int id = m_orderModel->data(index.siblingAtColumn(0)).toInt();
        // 삭제된 주문 정보를 status bar에 출력해주기 위해서 고객명과 제품명 가져오기
        std::string client = m_orderModel->data(index.siblingAtColumn(2)).toString().toStdString();
        std::string product = m_orderModel->data(index.siblingAtColumn(3)).toString().toStdString();
        m_orderModel->removeRow(index.row());
        m_orderModel->select();

        // 고객, 제품 상세 정보 초기화
        m_clientModel->removeRows(0, m_clientModel->rowCount());
        m_productModel->removeRows(0, m_productModel->rowCount());

        // status bar 메시지 출력
        emit sendStatusMessage(tr("delete completed (ID: %1, Client: %2, Product: %3)") \
                               .arg(id).arg(QString::fromStdString(client))\
                               .arg(QString::fromStdString(product)), 3000);
    }
}

/**
* @brief 고객 정보 관리 객체로부터 고객 정보를 받기 위한 슬롯
* @param int id 고객 ID
* @param std::string name 이름
* @param std::string phone 전화번호
* @param std::string address 주소
*/
void OrderManagerForm::receiveClientInfo(const int id, const std::string name, \
                                         const std::string phone, const std::string address)
{
    // 고객 상세 정보 model 초기화
    m_clientModel->removeRows(0, m_clientModel->rowCount());

    /* 고객 상세 정보 model에 고객 정보 추가 */
    std::vector<std::string> strings;
    strings.push_back(std::to_string(id));
    strings.push_back(name);
    strings.push_back(phone);
    strings.push_back(address);

    QList<QStandardItem *> items;
    for (const auto &i : strings) {
        items.append(new QStandardItem(QString::fromStdString(i)));
    }

    m_clientModel->appendRow(items);

    m_searchedClientFlag = true; // 제품 검색 결과를 true로 변경
}

/**
* @brief 제품 정보 관리 객체로부터 제품 정보를 받기 위한 슬롯
* @param int id
* @param std::string type
* @param std::string name
* @param int price
* @param int stock
*/
void OrderManagerForm::receiveProductInfo(const int id, const std::string type, \
                                          const std::string name, const int price, const int stock)
{
    // 제품 상세 정보 model 초기화
    m_productModel->removeRows(0, m_productModel->rowCount());

    /* 제품 상세 정보 model에 제품 정보 추가 */
    std::vector<std::string> strings;
    strings.push_back(std::to_string(id));
    strings.push_back(type);
    strings.push_back(name);
    strings.push_back(std::to_string(price));
    strings.push_back(std::to_string(stock));

    QList<QStandardItem *> items;
    for (const auto &i : strings) {
        items.append(new QStandardItem(QString::fromStdString(i)));
    }

    m_productModel->appendRow(items);

    m_searchedProductFlag = true; // 제품 검색 결과를 true로 변경
}

/**
* @brief 신규 주문 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int OrderManagerForm::makeId() const
{
    // id의 최댓값 가져오기
    QSqlQuery query("select count(*), max(id) from Order_list;",
                    m_orderModel->database());
    query.exec();
    while (query.next()) {
        // 등록된 주문이 없을 경우, id는 1000001부터 시작
        if(query.value(0).toInt() == 0)
            return 1000001;
        // 등록된 주문이 있을 경우, 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
        auto id = query.value(1).toInt();
        return ++id;
    }
    return 1;
}

/**
* @brief 입력 창 클리어
*/
void OrderManagerForm::cleanInputLineEdit() const
{
    m_ui->idLineEdit->clear();
    m_ui->dateEdit->setDate(QDate::currentDate());
    m_ui->clientLineEdit->clear();
    m_ui->productLineEdit->clear();
    m_ui->quantitySpinBox->clear();
}

/**
 * @brief tree view에서 주문을 클릭(선택)했을 때 실행되는 슬롯, 클릭된 주문의 정보를 입력 창에 표시
 * @param const QModelIndex &index 선택된 주문의 index
 */
void OrderManagerForm::on_treeView_clicked(const QModelIndex &index)
{
    /* 클릭된 주문의 정보를 가져와서 입력 창에 표시해줌 */
    std::string id = m_orderModel->data(index.siblingAtColumn(0)).toString().toStdString();
    std::string date = m_orderModel->data(index.siblingAtColumn(1)).toString().toStdString();
    std::string client = m_orderModel->data(index.siblingAtColumn(2)).toString().toStdString();
    std::string product = m_orderModel->data(index.siblingAtColumn(3)).toString().toStdString();
    int quantity = m_orderModel->data(index.siblingAtColumn(4)).toInt();

    m_ui->idLineEdit->setText(QString::fromStdString(id));
    m_ui->dateEdit->setDate(QDate::fromString(QString::fromStdString(date), "yyyy-MM-dd"));
    m_ui->clientLineEdit->setText(QString::fromStdString(client));
    m_ui->productLineEdit->setText(QString::fromStdString(product));
    m_ui->quantitySpinBox->setValue(quantity);

    /* 고객, 제품 상세 정보 tree view에 상세 정보 표시 */

    m_searchedClientFlag = false;  // 고객 검색 결과 flag 초기화
    m_searchedProductFlag = false; // 제품 검색 결과 flag 초기화

    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
    int clientId = std::stoi(client.substr(0, client.find(" ")));
    emit sendClientId(clientId);
    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
    int productId = std::stoi(product.substr(0, product.find(" ")));
    emit sendProductId(productId);

    m_searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
    m_searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
}
