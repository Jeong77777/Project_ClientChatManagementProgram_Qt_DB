#include "productmanagerform.h"
#include "ui_productmanagerform.h"

#include <QMenu>
#include <QMessageBox>
#include <QIntValidator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <cassert>

/**
* @brief 생성자, split 사이즈 설정, 입력 칸 설정, context 메뉴 설정, 검색 관련 초기 설정
*/
ProductManagerForm::ProductManagerForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProductManagerForm),
    menu(nullptr), productModel(nullptr)
{
    ui->setupUi(this);

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 170 << 400;
    ui->splitter->setSizes(sizes);

    /* 가격, 재고 입력 칸에 숫자만 입력 되도록 설정 */
    ui->priceLineEdit->setValidator( new QIntValidator(0, 999999999, this) );
    ui->stockLineEdit->setValidator( new QIntValidator(0, 999999999, this) );

    /* tree widget의 context 메뉴 설정 */
    // tree widget에서 고객을 삭제하는 action
    QAction* removeAction = new QAction(tr("Remove"));
    assert(connect(removeAction, SIGNAL(triggered()), SLOT(removeItem())));
    menu = new QMenu; // context 메뉴
    menu->addAction(removeAction);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    assert(connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)),\
            this, SLOT(showContextMenu(QPoint))));

    /* 검색 창에서 enter 키를 누르면 검색 버튼이 클릭되도록 connect */
    assert(connect(ui->searchLineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked())));

    /* 검색에서 제품 type을 선택하는 콤보 박스를 숨김 */
    // 실행 시 초기에는 검색할 항목이 type으로 지정되어 있지 않으므로 숨긴다.
    ui->searchTypeComboBox->hide();
}

/**
* @brief 제품 정보 데이터베이스 open
*/
void ProductManagerForm::loadData()
{
    /* 제품 정보 데이터베이스 open */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "productConnection");
    db.setDatabaseName("productlist.db");
    if (db.open( )) {
        // 제품 정보 테이블 생성
        QSqlQuery query(db);
        query.exec( "CREATE TABLE IF NOT EXISTS Product_list ("
                    "id          INTEGER          PRIMARY KEY, "
                    "type        VARCHAR(20)      NOT NULL,"
                    "name        VARCHAR(30)      NOT NULL,"
                    "price       INTEGER          NOT NULL,"
                    "stock       INTEGER          NOT NULL"
                    " )"
                    );

        // model로 데이터 베이스를 가져옴
        productModel = new QSqlTableModel(this, db);
        productModel->setTable("Product_list");
        productModel->select();
        productModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
        productModel->setHeaderData(1, Qt::Horizontal, tr("Type"));
        productModel->setHeaderData(2, Qt::Horizontal, tr("Name"));
        productModel->setHeaderData(3, Qt::Horizontal, tr("Unit Price"));
        productModel->setHeaderData(4, Qt::Horizontal, tr("Quantities in stock"));
        ui->treeView->setModel(productModel);
    }
}

/**
* @brief 소멸자, 제품 정보 데이터베이스 저장하고 닫기
*/
ProductManagerForm::~ProductManagerForm()
{
    delete ui;
    QSqlDatabase db = QSqlDatabase::database("productConnection");
    if(db.isOpen()) {
        productModel->submitAll();
        delete productModel;
        db.commit();
        db.close();
    }
}

/**
* @brief 전체 제품 리스트 출력 버튼 슬롯, tree view에 전체 제품 리스트를 출력해 준다.
*/
void ProductManagerForm::on_showAllPushButton_clicked()
{
    productModel->setFilter(""); // 필터 초기화
    productModel->select();
    ui->searchLineEdit->clear(); // 검색 창 클리어
}

/**
* @brief 검색 항목 선택 콤보 박스에서 선택된 것이 변경되었을 때 실행되는 슬롯
* @Param int index 선택된 항목의 index
*/
void ProductManagerForm::on_searchComboBox_currentIndexChanged(int index)
{
    // 0. ID  1. Type  2. Name
    if(index == 1) { // 검색 항목으로 Type을 선택 했을 때
        ui->searchLineEdit->hide();     // line edit를 숨겨준다.
        ui->searchTypeComboBox->show(); // Type 선택 콤보 박스를 보여준다.
    }
    else { // 검색 항목으로 ID 또는 Name을 선택 했을 때
        ui->searchTypeComboBox->hide(); // Type 선택 콤보 박스를 숨겨준다.
        ui->searchLineEdit->show();     // line edit를 보여준다.
    }
}

/**
* @brief 검색 버튼 슬롯, tree view에 검색 결과를 출력해 준다.
*/
void ProductManagerForm::on_searchPushButton_clicked()
{
    /* 현재 선택된 검색 항목 */
    // 0. ID  1. Type  2. Name
    int i = ui->searchComboBox->currentIndex();

    /* 검색 수행 */
    std::string str; // 검색어
    if(i == 1) // Type
        str = ui->searchTypeComboBox->currentText().toStdString();
    else {     // ID, Name
        str = ui->searchLineEdit->text().toStdString();
        if(!str.length()) { // 검색 창이 비어 있을 때
            QMessageBox::warning(this, tr("Search error"),
                                 tr("Please enter a search term."), QMessageBox::Ok);
            return;
        }
    }

    // 0. ID  1. Type  2. Name
    switch (i) {
    case 0: productModel->setFilter(QString("id = '%1'").arg(QString::fromStdString(str)));
        break;
    case 1: productModel->setFilter(QString("type = '%1'").arg(QString::fromStdString(str)));
        break;
    case 2: productModel->setFilter(QString("name LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    default:
        break;
    }
    productModel->select();

    // status bar 메시지 출력
    emit sendStatusMessage(tr("%1 search results were found").arg(productModel->rowCount()), 3000);

    /* 사용자가 정보를 변경해도 검색 결과가 유지되도록 ID를 이용해서 필터 재설정 */
    std::string filterStr = "id in (";
    for(int i = 0; i < productModel->rowCount(); i++) {
        int id = productModel->data(productModel->index(i, 0)).toInt();
        if(i != productModel->rowCount()-1)
            filterStr += (std::to_string(id) + ", ");
        else
            filterStr += std::to_string(id);
    }
    filterStr += ");";
    qDebug() << QString::fromStdString(filterStr);
    productModel->setFilter(QString::fromStdString(filterStr));
}

/**
* @brief 제품 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 제품을 추가함
*/
void ProductManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    std::string type, name, price, stock;
    int id = makeId(); // 자동으로 ID 생성
    type = ui->typeComboBox->currentText().toStdString();
    name = ui->nameLineEdit->text().toStdString();
    price = ui->priceLineEdit->text().toStdString();
    stock = ui->stockLineEdit->text().toStdString();

    /* 입력된 정보를 DB에 추가 */
    QSqlDatabase db = QSqlDatabase::database("productConnection");
    if(db.isOpen() && name.length() && price.length() && stock.length()) {
        QSqlQuery query(productModel->database());
        query.prepare( "INSERT INTO Product_list "
                       "(id, type, name, price, stock) "
                       "VALUES "
                       "(:ID, :TYPE, :NAME, :PRICE, :STOCK)" );
        query.bindValue(":ID",        id);
        query.bindValue(":TYPE",      QString::fromStdString(type));
        query.bindValue(":NAME",      QString::fromStdString(name));
        query.bindValue(":PRICE",     QString::fromStdString(price));
        query.bindValue(":STOCK",     QString::fromStdString(stock));
        query.exec();
        productModel->select();

        cleanInputLineEdit(); // 입력 창 클리어

        // status bar 메시지 출력
        emit sendStatusMessage(tr("Add completed (ID: %1, Name: %2)")\
                               .arg(id).arg(QString::fromStdString(name)), 3000);
    }
    else { // 비어있는 입력 창이 있을 때
        QMessageBox::warning(this, tr("Add error"),
                             QString(tr("Some items have not been entered.")), \
                             QMessageBox::Ok);
    }
}

/**
* @brief 제품 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 제품 정보를 변경함
*/
void ProductManagerForm::on_modifyPushButton_clicked()
{
    /* tree view에서 현재 선택된 고객의 index 가져오기 */
    QModelIndex index = ui->treeView->currentIndex();

    /* 입력 창에 입력된 정보에 따라 제품 정보를 변경 */
    if(index.isValid()) {
        // 입력 창에 입력된 정보 가져오기
        int id = productModel->data(index.siblingAtColumn(0)).toInt();
        std::string type, name, price, stock;
        type = ui->typeComboBox->currentText().toStdString();
        name = ui->nameLineEdit->text().toStdString();
        price = ui->priceLineEdit->text().toStdString();
        stock = ui->stockLineEdit->text().toStdString();

        // 입력 창에 입력된 정보에 따라 제품 정보를 변경
        if(name.length() && price.length() && stock.length()) {
            QSqlQuery query(productModel->database());
            query.prepare("UPDATE Product_list SET type = ?, "
                          "name = ?, price = ?, stock = ? WHERE id = ?");
            query.bindValue(0, QString::fromStdString(type));
            query.bindValue(1, QString::fromStdString(name));
            query.bindValue(2, QString::fromStdString(price));
            query.bindValue(3, QString::fromStdString(stock));
            query.bindValue(4, id);
            query.exec();
            productModel->select();

            cleanInputLineEdit(); // 입력 창 클리어

            // status bar 메시지 출력
            emit sendStatusMessage(tr("Modify completed (ID: %1, Name: %2)") \
                                   .arg(id).arg(QString::fromStdString(name)), 3000);
        }
        else { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Modify error"), \
                                 QString(tr("Some items have not been entered.")), \
                                 QMessageBox::Ok);
        }
    }
}

/**
* @brief 입력 창 클리어 버튼 슬롯, 입력 창을 클리어 하는 함수 호출
*/
void ProductManagerForm::on_cleanPushButton_clicked()
{
    cleanInputLineEdit();
}

/**
* @brief tree view의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void ProductManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree view 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeView->mapToGlobal(pos);
    if(ui->treeView->indexAt(pos).isValid())
        menu->exec(globalPos);
}

/**
* @brief 제품 정보 삭제
*/
void ProductManagerForm::removeItem()
{
    /* tree view에서 현재 선택된 제품의 index 가져오기 */
    QModelIndex index = ui->treeView->currentIndex();

    /* 제품 정보 삭제 */
    if(index.isValid()) {
        // 삭제된 제품 정보를 status bar에 출력해주기 위해서 id와 이름 가져오기
        int id = productModel->data(index.siblingAtColumn(0)).toInt();
        std::string name = productModel->data(index.siblingAtColumn(2)).toString().toStdString();

        // 제품 정보 삭제
        productModel->removeRow(index.row());
        productModel->select();

        // status bar 메시지 출력
        emit sendStatusMessage(tr("delete completed (ID: %1, Name: %2)") \
                               .arg(id).arg(QString::fromStdString(name)), 3000);
    }
}

/**
* @brief 주문 정보 관리 객체에서 제품ID를 가지고 제품을 검색 하기 위한 슬롯
* @Param int id 검색할 제품 id
*/
void ProductManagerForm::receiveId(int id)
{    
    /* 제품 ID를 이용하여 제품 검색 */
    QSqlQuery query(QString("select * "
                            "from Product_list "
                            "where id = '%1';").arg(id),
                    productModel->database());
    query.exec();
    while (query.next()) {
        int id = query.value(0).toInt();
        std::string type = query.value(1).toString().toStdString();
        std::string name = query.value(2).toString().toStdString();
        int price = query.value(3).toInt();
        int stock = query.value(4).toInt();

        // 검색 결과를 주문 정보 관리 객체로 보냄
        emit sendProductToManager(id, type, name, price, stock);
    }
}

/**
* @brief 제품 검색 Dialog에서 제품을 검색 하기 위한 슬롯
* @Param std::string word 검색어(id 또는 이름)
*/
void ProductManagerForm::receiveWord(std::string word)
{
    /* 제품 ID 또는 이름을 이용하여 고객 검색 */
    QSqlQuery query(QString("select * "
                            "from Product_list "
                            "where id = '%1' "
                            "or name LIKE '%%1%';").arg(QString::fromStdString(word)),
                    productModel->database());
    query.exec();
    while (query.next()) {
        int id = query.value(0).toInt();
        std::string type = query.value(1).toString().toStdString();
        std::string name = query.value(2).toString().toStdString();
        int price = query.value(3).toInt();
        int stock = query.value(4).toInt();

        // 검색 결과를 제품 검색 Dialog로 보냄
        emit sendProductToDialog(id, type, name, price, stock);
    }
}

/**
* @brief 신규 제품 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int ProductManagerForm::makeId()
{
    // id의 최댓값 가져오기
    QSqlQuery query("select count(*), max(id) from Product_list;",
                    productModel->database());
    query.exec();
    while (query.next()) {
        if(query.value(0).toInt() == 0) // 등록된 고객이 없을 경우
            return 1001;                // id는 1001부터 시작
        else {                          // 등록된 고객이 있을 경우
            auto id = query.value(1).toInt();
            return ++id;                // 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
        }
    }
    return 1;
}

/**
* @brief 입력 창 클리어
*/
void ProductManagerForm::cleanInputLineEdit()
{
    ui->idLineEdit->clear();
    ui->nameLineEdit->clear();
    ui->priceLineEdit->clear();
    ui->stockLineEdit->clear();
}

/**
* @brief tree view에서 제품을 클릭(선택)했을 때 실행되는 슬롯, 클릭된 제품의 정보를 입력 창에 표시
* @param const QModelIndex &index 선택된 제품의 index
*/
void ProductManagerForm::on_treeView_clicked(const QModelIndex &index)
{
    /* 클릭된 제품의 정보를 가져와서 입력 창에 표시해줌 */
    std::string id = productModel->data(index.siblingAtColumn(0)).toString().toStdString();
    std::string type = productModel->data(index.siblingAtColumn(1)).toString().toStdString();
    std::string name = productModel->data(index.siblingAtColumn(2)).toString().toStdString();
    std::string price = productModel->data(index.siblingAtColumn(3)).toString().toStdString();
    std::string stock = productModel->data(index.siblingAtColumn(4)).toString().toStdString();

    ui->idLineEdit->setText(QString::fromStdString(id));
    ui->typeComboBox->setCurrentText(QString::fromStdString(type));
    ui->nameLineEdit->setText(QString::fromStdString(name));
    ui->priceLineEdit->setText(QString::fromStdString(price));
    ui->stockLineEdit->setText(QString::fromStdString(stock));
}

/**
* @brief 주문 정버 관리 객체를 통해서 재고량을 변경하기 위한 슬롯
* @param int id 재고를 변경할 제품의 ID
* @param int stock 변경할 재고수량
*/
void ProductManagerForm::setStock(int id, int stock)
{
    if(stock >= 0) {
        QSqlQuery query(productModel->database());
        query.prepare("UPDATE Product_list SET stock = ? WHERE id = ?");
        query.bindValue(0, stock);
        query.bindValue(1, id);
        query.exec();
        productModel->select();
    }
}
