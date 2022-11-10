#include "productmanagerform.h"
#include "ui_productmanagerform.h"

#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QIntValidator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QTreeWidgetItem>

/**
* @brief 생성자, split 사이즈 설정, 입력 칸 설정, context 메뉴 설정, 검색 관련 초기 설정
*/
ProductManagerForm::ProductManagerForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProductManagerForm)
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
    connect(removeAction, SIGNAL(triggered()), SLOT(removeItem()));
    menu = new QMenu; // context 메뉴
    menu->addAction(removeAction);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)),\
            this, SLOT(showContextMenu(QPoint)));

    /* 검색 창에서 enter 키를 누르면 검색 버튼이 클릭되도록 connect */
    connect(ui->searchLineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked()));

    /* 검색에서 제품 type을 선택하는 콤보 박스를 숨김 */
    // 실행 시 초기에는 검색할 항목이 type으로 지정되어 있지 않으므로 숨긴다.
    ui->searchTypeComboBox->hide();
}

/**
* @brief productList.txt 파일을 열어서 저장된 제품 리스트를 가져옴
*/
void ProductManagerForm::loadData()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "productConnection");
    db.setDatabaseName("productlist.db");
    if (db.open( )) {
        QSqlQuery query(db);
        query.exec( "CREATE TABLE IF NOT EXISTS Product_list ("
                    "id          INTEGER          PRIMARY KEY, "
                    "type        VARCHAR(30)      NOT NULL,"
                    "name        VARCHAR(20)      NOT NULL,"
                    "price       INTEGER          NOT NULL,"
                    "stock       INTEGER          NOT NULL"
                    " )"
                    );

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
* @brief 소멸자,
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
    productModel->setFilter("");
    productModel->select();
    ui->searchLineEdit->clear(); // 검색 창 클리어
}

/**
* @brief 검색 항목 선택 콤보 박스에서 선택된 것이 변경되었을 때 실행되는 슬롯
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
    QString str; // 검색어
    if(i == 1) // Type
        str = ui->searchTypeComboBox->currentText();
    else {     // ID, Name
        str = ui->searchLineEdit->text();
        if(!str.length()) { // 검색 창이 비어 있을 때
            QMessageBox::warning(this, tr("Search error"),
                                     tr("Please enter a search term."), QMessageBox::Ok);
            return;
        }
    }

    switch (i) {
    case 0: productModel->setFilter(QString("id = '%1'").arg(str));
        break;
    case 1: productModel->setFilter(QString("type = '%1'").arg(str));
        break;
    case 2: productModel->setFilter(QString("name LIKE '%%1%'").arg(str));
        break;
    default:
        break;
    }
    productModel->select();
    emit sendStatusMessage(tr("%1 search results were found").arg(productModel->rowCount()), 3000);

    QString filterStr = "id in (";
    for(int i = 0; i < productModel->rowCount(); i++) {
        int id = productModel->data(productModel->index(i, 0)).toInt();
        if(i != productModel->rowCount()-1)
            filterStr += QString("%1, ").arg(id);
        else
            filterStr += QString("%1").arg(id);
    }
    filterStr += ");";
    qDebug() << filterStr;
    productModel->setFilter(filterStr);
}


/**
* @brief 제품 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 제품을 추가함
*/
void ProductManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    QString type, name, price, stock;
    int id = makeId(); // 자동으로 ID 생성
    type = ui->typeComboBox->currentText();
    name = ui->nameLineEdit->text();
    price = ui->priceLineEdit->text();
    stock = ui->stockLineEdit->text();

    /* 입력된 정보를 DB에 추가 */
    QSqlDatabase db = QSqlDatabase::database("productConnection");
    if(db.isOpen() && name.length() && price.length() && stock.length()) {
        QSqlQuery query(productModel->database());
        query.prepare( "INSERT INTO Product_list "
                       "(id, type, name, price, stock) "
                       "VALUES "
                       "(:ID, :TYPE, :NAME, :PRICE, :STOCK)" );
        query.bindValue(":ID",        id);
        query.bindValue(":TYPE",      type);
        query.bindValue(":NAME",      name);
        query.bindValue(":PRICE",     price);
        query.bindValue(":STOCK",     stock);
        query.exec();
        productModel->select();

        cleanInputLineEdit(); // 입력 창 클리어

        emit sendStatusMessage(tr("Add completed (ID: %1, Name: %2)").arg(id).arg(name), 3000);
    }
    else { // 비어있는 입력 창이 있을 때
        QMessageBox::warning(this, tr("Add error"),
                             QString(tr("Some items have not been entered.")), QMessageBox::Ok);
    }
}

/**
* @brief 제품 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 제품 정보를 변경함
*/
void ProductManagerForm::on_modifyPushButton_clicked()
{
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = productModel->data(index.siblingAtColumn(0)).toInt();
        // 입력 창에 입력된 정보 가져오기
        QString type, name, price, stock;
        type = ui->typeComboBox->currentText();
        name = ui->nameLineEdit->text();
        price = ui->priceLineEdit->text();
        stock = ui->stockLineEdit->text();

        // 입력 창에 입력된 정보에 따라 제품 정보를 변경
        if(name.length() && price.length() && stock.length()) {
            QSqlQuery query(productModel->database());
            query.prepare("UPDATE Client_list SET type = ?, name = ?, price = ?, stock = ?, WHERE id = ?");
            query.bindValue(0, type);
            query.bindValue(1, name);
            query.bindValue(2, price);
            query.bindValue(3, stock);
            query.bindValue(4, id);
            query.exec();
            productModel->select();

            cleanInputLineEdit(); // 입력 창 클리어

            emit sendStatusMessage(tr("Modify completed (ID: %1, Name: %2)").arg(id).arg(name), 3000);
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
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = productModel->data(index.siblingAtColumn(0)).toInt();
        QString name = productModel->data(index.siblingAtColumn(1)).toString();
        productModel->removeRow(index.row());
        productModel->select();
        emit sendStatusMessage(tr("delete completed (ID: %1, Name: %2)").arg(id).arg(name), 3000);
    }
}

/**
* @brief 주문 정보 관리 객체에서 제품ID를 가지고 제품을 검색 하기 위한 슬롯
* @Param int id 검색할 제품 id
*/
void ProductManagerForm::receiveId(int id)
{
    QSqlQuery query(QString("select * "
                            "from Product_list "
                            "where id = '%1';").arg(id),
                    productModel->database());
    query.exec();
    while (query.next()) {
        int id = query.value(0).toInt();
        QString type = query.value(1).toString();
        QString name = query.value(2).toString();
        QString price = query.value(3).toString();
        QString stock = query.value(4).toString();

        QTreeWidgetItem* item  = new QTreeWidgetItem;
        item->setText(0, QString::number(id));
        item->setText(1, type);
        item->setText(2, name);
        item->setText(3, price);
        item->setText(4, stock);

        //검색 결과를 주문 정보 관리 객체로 보냄
        emit sendProductToManager(item);
    }
}

/**
* @brief 제품 검색 Dialog에서 제품을 검색 하기 위한 슬롯
* @Param QString word 검색어(id 또는 이름)
*/
void ProductManagerForm::receiveWord(QString word)
{ // 여기서부터 하기
    QSqlQuery query(QString("select * "
                            "from Product_list "
                            "where id = '%1' "
                            "or name LIKE '%%1%' "
                            "or phoneNumber LIKE '%%1%' "
                            "or address LIKE '%%1%';").arg(word),
                    productModel->database());
    query.exec();
    while (query.next()) {
        qDebug() << query.value(0).toInt();
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        QString phone = query.value(2).toString();
        QString address = query.value(3).toString();
        qDebug() << id << name;
        QTreeWidgetItem* item  = new QTreeWidgetItem;
        item->setText(0, QString::number(id));
        item->setText(1, name);
        item->setText(2, phone);
        item->setText(3, address);
        emit sendProductToDialog(item);
    }
}

/**
* @brief 신규 제품 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int ProductManagerForm::makeId()
{
//    if(productList.size( ) == 0) {
//        return 1001; // id는 1001부터 시작
//    } else {
//        auto id = productList.lastKey();
//        return ++id; // 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
//    }
    return 0;
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
