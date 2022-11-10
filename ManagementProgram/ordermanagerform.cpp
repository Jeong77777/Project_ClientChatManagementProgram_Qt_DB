#include "ordermanagerform.h"
#include "ui_ordermanagerform.h"
#include "orderitem.h"
#include "clientdialog.h"
#include "productdialog.h"
#include "productitem.h"

#include <QFile>
#include <QMenu>
#include <QMessageBox>

/**
* @brief 생성자, split 사이즈 설정, 입력 칸 초기화, context 메뉴 설정, 검색 관련 초기 설정
*/
OrderManagerForm::OrderManagerForm(QWidget *parent, \
                                   ClientDialog *clientDialog, ProductDialog *productDialog) :
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
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), \
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
    /* orderlist.txt 파일을 연다. */
    QFile file("orderlist.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    /* parsing 후 주문 정보를 tree widget에 추가 */
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QList<QString> row = line.split(", ");
        if(row.size()) {
            int id = row[0].toInt();
            OrderItem* o = new OrderItem(id, row[1], row[2].toInt(), row[3],
                    row[4].toInt(), row[5], row[6].toInt(), row[7]);
            ui->treeWidget->addTopLevelItem(o);
            orderList.insert(id, o);
        }
    }
    file.close( );
}

/**
* @brief 소멸자, 주문 리스트를 orderlist.txt에 저장
*/
OrderManagerForm::~OrderManagerForm()
{
    delete ui;

    /* orderlist.txt 파일을 연다. */
    QFile file("orderlist.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    /* 구분자를 ", "로 해서 주문 정보를 파일에 저장 */
    QTextStream out(&file);
    for (const auto& v : qAsConst(orderList)) {
        OrderItem* o = v;
        out << o->id() << ", " << o->getDate() << ", ";
        out << o->getClinetId() << ", " << o->getClientName() << ", ";
        out << o->getProductId() << ", " << o->getProductName() << ", ";
        out << o->getQuantity() << ", " << o->getTotal() << "\n";
    }
    file.close( );
}


/**
* @brief 전체 주문 리스트 출력 버튼 슬롯, tree widget에 전체 주문 리스트를 출력해 준다.
*/
void OrderManagerForm::on_showAllPushButton_clicked()
{
    for (const auto& v : qAsConst(orderList)) {
        v->setHidden(false);
    }
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
    // 2 3: 대소문자 구분, 부분 일치 검색, 0 1: 대소문자 구분 검색
    auto flag = (i >= 2)? Qt::MatchCaseSensitive|Qt::MatchContains
                        : Qt::MatchCaseSensitive;

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

    // 검색
    auto items = ui->treeWidget->findItems(str, flag, i);

    /* 검색된 결과만 tree widget에 보여 주기*/
    for (const auto& v : qAsConst(orderList))
        v->setHidden(true);
    foreach(auto i, items)
        i->setHidden(false);

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
        ProductItem* p = dynamic_cast<ProductItem*>(productDialog->getCurrentItem());
        if(p!=nullptr) {
            // 주문 정보 입력 칸에 제품 ID와 이름을 입력한다.
            ui->productLineEdit->setText(QString::number(p->id()) + " (" + p->getName() + ")");
        }
    }
    productDialog->clearDialog(); // 다이얼로그 초기화
}

/**
* @brief 주문 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문을 추가함
*/
void OrderManagerForm::on_addPushButton_clicked()
{
//    /* 입력 창에 입력된 정보 가져오기 */
//    QString date, clientName, productName, total;
//    int id = makeId(); // 자동으로 ID 생성
//    int clientId, productId, quantity;
//    clientId = ui->clientLineEdit->text().split(" ")[0].toInt();
//    productId = ui->productLineEdit->text().split(" ")[0].toInt();
//    quantity = ui->quantitySpinBox->text().toInt();
//    clientName = ui->clientLineEdit->text();
//    productName = ui->productLineEdit->text();
//    date = ui->dateEdit->date().toString("yyyy-MM-dd");

//    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
//    emit sendClientId(clientId);
//    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
//    emit sendProductId(productId);

//    /* 입력된 정보로 tree widget item을 생성하고 tree widget에 추가 */
//    try {
//        if(searchedClientFlag == false)                // 고객 정보가 존재하지 않을 때
//            throw tr("Customer information does not exist.");
//        else if(searchedProductFlag == false)          // 제품 정보가 존재하지 않을 때
//            throw tr("Product information does not exist.");
//        else if(quantity == 0)                         // 올바른 수량을 입력하지 않았을 때
//            throw tr("Please enter a valid quantity.");
//        else if(searchedProduct->getStock() < quantity) // 재고가 부족할 때
//            throw tr("There is a shortage of stock.");

//        // 주문 금액 계산
//        total = QString::number(quantity * searchedProduct->getPrice());

//        // order tree widget item 객체 생성
//        OrderItem* o = new OrderItem(id, date, clientId, clientName,
//                                     productId, productName, quantity, total);
//        orderList.insert(id, o);            // 주문 리스트에 추가
//        ui->treeWidget->addTopLevelItem(o); // tree widget에 추가

//        // 주문 수량만큼 제품의 재고 차감
//        searchedProduct->setStock(searchedProduct->getStock() - quantity);

//        cleanInputLineEdit(); // 입력 창 클리어

//    } catch (QString msg) { // 비어있는 입력 창이 있을 때
//        QMessageBox::warning(this, tr("Add error"),
//                                 QString(msg), QMessageBox::Ok);
//    }

//    searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
//    searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
}

/**
* @brief 주문 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 주문 정보를 변경함
*/
void OrderManagerForm::on_modifyPushButton_clicked()
{
//    /* tree widget에서 현재 선택된 item 가져오기 */
//    QTreeWidgetItem* item = ui->treeWidget->currentItem();

//    /* 입력 창에 입력된 정보에 따라 주문 정보를 변경 */
//    if(item != nullptr) {
//        // ID를 이용하여 주문 리스트에서 주문 가져오기
//        int key = item->text(0).toInt();
//        OrderItem* o = orderList[key];

//        // 입력 창에 입력된 정보 가져오기
//        QString date, clientName, productName, total;
//        int clientId, productId, oldQuantity, newQuantity;
//        clientId = ui->clientLineEdit->text().split(" ")[0].toInt();
//        productId = ui->productLineEdit->text().split(" ")[0].toInt();
//        oldQuantity = o->getQuantity();                    // 변경 전 주문 수량
//        newQuantity = ui->quantitySpinBox->text().toInt(); // 변경 후 주문 수량
//        clientName = ui->clientLineEdit->text();
//        productName = ui->productLineEdit->text();
//        date = ui->dateEdit->date().toString("yyyy-MM-dd");

//        // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
//        emit sendClientId(clientId);
//        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
//        emit sendProductId(productId);

//        /* 입력 창에 입력된 정보에 따라 주문 정보를 변경 */
//        try {
//            if(searchedClientFlag == false)       // 고객 정보가 존재하지 않을 때
//                throw tr("Customer information does not exist.");
//            else if(searchedProductFlag == false) // 제품 정보가 존재하지 않을 때
//                throw tr("Product information does not exist.");
//            else if(newQuantity == 0)             // 올바른 수량을 입력하지 않았을 때
//                throw tr("Please enter a valid quantity.");
//            else if(searchedProduct->getStock() + oldQuantity \
//                    < newQuantity)                // 재고가 부족할 때
//                throw tr("There is a shortage of stock.\n") + tr("Maximum: ")
//                    + QString::number(searchedProduct->getStock() + oldQuantity);

//            // 주문 금액 계산
//            total = QString::number(newQuantity * searchedProduct->getPrice());

//            // 주문 수량만큼 제품의 재고 차감
//            searchedProduct->setStock(searchedProduct->getStock() + oldQuantity - newQuantity);

//            // 입력 창에 입력된 정보에 따라 주문 정보를 변경
//            o->setDate(date);
//            o->setClientId(clientId);
//            o->setClientName(clientName);
//            o->setProductId(productId);
//            o->setProductName(productName);
//            o->setQuantity(newQuantity);
//            o->setTotal(total);
//            orderList[key] = o;

//            searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
//            searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경

//            // 주문 정보가 변경 됨에 따라
//            // 고객, 제품 상세 정보 tree widget에 새로운 정보 표시를 위해서 다음의 함수 호출
//            on_treeWidget_itemClicked(item, 0);
//        }
//        catch (QString msg) { // 비어있는 입력 창이 있을 때
//            QMessageBox::warning(this, tr("Add error"),
//                                     QString(msg), QMessageBox::Ok);
//        }

//        searchedClientFlag = false;  // 고객 검색 결과 flag를 다시 false로 변경
//        searchedProductFlag = false; // 제품 검색 결과 flag를 다시 false로 변경
//    }
}

/**
* @brief 입력 창 클리어 버튼 슬롯, 입력 창을 클리어 하는 함수 호출
*/
void OrderManagerForm::on_cleanPushButton_clicked()
{
    cleanInputLineEdit();
}

/**
* @brief tree widget에서 제품을 클릭(선택)했을 때 실행되는 슬롯
* @brief 클릭된 제품의 정보를 입력 창에 표시, 고객, 제품 상세 정보 tree widget에 상세 정보 표시
* @Param QTreeWidgetItem *item 클릭된 item
* @Param int column 클릭된 item의 열
*/
void OrderManagerForm::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    /* 클릭된 제품의 정보를 입력 창에 표시해줌 */
    ui->idLineEdit->setText(item->text(0));
    ui->dateEdit->setDate(QDate::fromString(item->text(1), "yyyy-MM-dd"));
    ui->clientLineEdit->setText(item->text(2));
    ui->productLineEdit->setText(item->text(3));
    ui->quantitySpinBox->setValue(item->text(4).toInt());

    searchedClientFlag = false;  // 고객 검색 결과 flag 초기화
    searchedProductFlag = false; // 제품 검색 결과 flag 초기화

    /* 고객, 제품 상세 정보 tree widget에 상세 정보 표시 */

    // 고객ID를 이용해서 고객 정보 관리 객체로부터 고객 가져오기
    int clientId = ui->clientLineEdit->text().split(" ")[0].toInt();
    emit sendClientId(clientId);
    // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
    int productId = ui->productLineEdit->text().split(" ")[0].toInt();
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

/**
* @brief tree widget의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void OrderManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree widget 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeWidget->mapToGlobal(pos);
    menu->exec(globalPos);
}

/**
* @brief 고객 정보 삭제
*/
void OrderManagerForm::removeItem()
{
    /* tree widget에서 현재 선택된 item 가져오기 */
    QTreeWidgetItem* item = ui->treeWidget->currentItem();

    /* 고객 정보 삭제 */
    if(item != nullptr) {

        // 삭제된 재고를 다시 추가하기 위해서
        // 제품ID를 이용해서 제품 정보 관리 객체로부터 제품 가져오기
        int productId = item->text(3).split(" ")[0].toInt();
        emit sendProductId(productId);
        if(searchedProductFlag == true) { // 제품 정보가 존재할 때
            // OK를 누르면 삭제된 재고를 다시 추가
            if(QMessageBox::Yes == QMessageBox::information(this, tr("Order list remove"), \
                                                            tr("Do you want to re-add the deleted inventory?"), \
                                                            QMessageBox::Yes|QMessageBox::No))
                ;//searchedProduct->setText(4, searchedProduct->text(4) + item->text(4).toInt());
        }
        searchedProductFlag = false; // 고객 검색 결과 flag를 다시 false로 변경

        orderList.remove(item->text(0).toInt()); // 리스트에서 고객 정보 삭제
        // tree widget에서 고객 정보 삭제
        ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(item));
        delete item;

        ui->treeWidget->update();       // tree widget update
        ui->clientTreeWidget->clear();  // 고객 상세 정보 클리어
        ui->productTreeWidget->clear(); // 제품 상세 정보 클리어
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
    if(orderList.size( ) == 0) {
        return 1000001; // id는 1000001부터 시작
    } else {
        auto id = orderList.lastKey();
        return ++id; // 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
    }
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
