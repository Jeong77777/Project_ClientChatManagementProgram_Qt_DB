#include "productmanagerform.h"
#include "ui_productmanagerform.h"
#include "productitem.h"

#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QIntValidator>

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
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)),\
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
    /* productList.txt 파일을 연다. */
    QFile file("productList.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    /* parsing 후 제품 정보를 tree widget에 추가 */
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QList<QString> row = line.split(", ");
        if(row.size()) {
            int id = row[0].toInt();
            ProductItem* c = new ProductItem(id, row[1], row[2], \
                    row[3].toInt(), row[4].toInt());
            ui->treeWidget->addTopLevelItem(c);
            productList.insert(id, c);
        }
    }
    file.close( );
}

/**
* @brief 소멸자, 제품 리스트를 productList.txt에 저장
*/
ProductManagerForm::~ProductManagerForm()
{
    delete ui;

    /* productList.txt 파일을 연다. */
    QFile file("productList.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    /* 구분자를 ", "로 해서 제품 정보를 파일에 저장 */
    QTextStream out(&file);
    for (const auto& v : qAsConst(productList)) {
        ProductItem* c = v;
        out << c->id() << ", " << c->getType() << ", ";
        out << c->getName() << ", " << c->getPrice() << ", ";
        out << c->getStock() << "\n";
    }
    file.close( );
}

/**
* @brief 전체 제품 리스트 출력 버튼 슬롯, tree widget에 전체 제품 리스트를 출력해 준다.
*/
void ProductManagerForm::on_showAllPushButton_clicked()
{
    for (const auto& v : qAsConst(productList)) {
        v->setHidden(false);
    }
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
* @brief 검색 버튼 슬롯, tree widget에 검색 결과를 출력해 준다.
*/
void ProductManagerForm::on_searchPushButton_clicked()
{
    /* 현재 선택된 검색 항목 */
    // 0. ID  1. Type  2. Name
    int i = ui->searchComboBox->currentIndex();

    /* 검색 수행 */
    // 2: 대소문자 구분, 부분 일치 검색, 0 1: 대소문자 구분 검색
    auto flag = (i == 2)? Qt::MatchCaseSensitive|Qt::MatchContains
                        : Qt::MatchCaseSensitive;

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

    // 검색
    auto items = ui->treeWidget->findItems(str, flag, i);

    /* 검색된 결과만 tree widget에 보여 주기*/
    for (const auto& v : qAsConst(productList))
        v->setHidden(true);
    foreach(auto i, items)
        i->setHidden(false);
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

    /* 입력된 정보로 tree widget item을 생성하고 tree widget에 추가 */
    if(name.length() && price.length() && stock.length()) {
        ProductItem* p = new ProductItem(id, type, name, \
                                         price.toInt(), stock.toInt());
        productList.insert(id, p);          // 제품 리스트에 추가
        ui->treeWidget->addTopLevelItem(p); // tree widget에 추가

        cleanInputLineEdit(); // 입력 창 클리어
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
    /* tree widget에서 현재 선택된 item 가져오기 */
    QTreeWidgetItem* item = ui->treeWidget->currentItem();

    /* 입력 창에 입력된 정보에 따라 제품 정보를 변경 */
    if(item != nullptr) {
        // ID를 이용하여 제품 리스트에서 제품 가져오기
        int key = item->text(0).toInt();
        ProductItem* p = productList[key];

        // 입력 창에 입력된 정보 가져오기
        QString type, name, price, stock;
        type = ui->typeComboBox->currentText();
        name = ui->nameLineEdit->text();
        price = ui->priceLineEdit->text();
        stock = ui->stockLineEdit->text();

        // 입력 창에 입력된 정보에 따라 제품 정보를 변경
        if(name.length() && price.length() && stock.length()) {
            p->setType(type);
            p->setName(name);
            p->setPrice(price.toInt());
            p->setStock(stock.toInt());
            productList[key] = p;
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
* @brief tree widget에서 제품을 클릭(선택)했을 때 실행되는 슬롯, 클릭된 제품의 정보를 입력 창에 표시
* @Param QTreeWidgetItem *item 클릭된 item
* @Param int column 클릭된 item의 열
*/
void ProductManagerForm::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    /* 클릭된 제품의 정보를 입력 창에 표시해줌 */
    ui->idLineEdit->setText(item->text(0));
    ui->typeComboBox->setCurrentText(item->text(1));
    ui->nameLineEdit->setText(item->text(2));
    ui->priceLineEdit->setText(item->text(3));
    ui->stockLineEdit->setText(item->text(4));
}

/**
* @brief tree widget의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void ProductManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree widget 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeWidget->mapToGlobal(pos);
    menu->exec(globalPos);
}

/**
* @brief 제품 정보 삭제
*/
void ProductManagerForm::removeItem()
{
    /* tree widget에서 현재 선택된 item 가져오기 */
    QTreeWidgetItem* item = ui->treeWidget->currentItem();

    /* 제품 정보 삭제 */
    if(item != nullptr) {
        productList.remove(item->text(0).toInt()); // 리스트에서 제품 정보 삭제
        // tree widget에서 제품 정보 삭제
        ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(item));
        delete item;
        ui->treeWidget->update(); // tree widget update
    }
}

/**
* @brief 주문 정보 관리 객체에서 제품ID를 가지고 제품을 검색 하기 위한 슬롯
* @Param int id 검색할 제품 id
*/
void ProductManagerForm::receiveId(int id)
{
    for (const auto& v : qAsConst(productList)) {
        ProductItem* p = v;
        if(p->id() == id) {
            // 검색 결과를 주문 정보 관리 객체로 보냄
            emit sendProductToManager(p);
        }
    }
}

/**
* @brief 제품 검색 Dialog에서 제품을 검색 하기 위한 슬롯
* @Param QString word 검색어(id 또는 이름)
*/
void ProductManagerForm::receiveWord(QString word)
{
    /* 검색 결과를 저장할 map */
    QMap<int, ProductItem*> searchList;

    /* 대소문자를 구분하고 부분 일치 검색으로 설정 */
    auto flag = Qt::MatchCaseSensitive|Qt::MatchContains;

    /* id에서 검색 */
    auto items1 = ui->treeWidget->findItems(word, flag, 0);
    foreach(auto i, items1) {
        ProductItem* p = static_cast<ProductItem*>(i);
        searchList.insert(p->id(), p); // 검색 결과를 map에 저장
    }

    /* 이름에서 검색 */
    auto items2 = ui->treeWidget->findItems(word, flag, 2);
    foreach(auto i, items2) {
        ProductItem* p = static_cast<ProductItem*>(i);
        searchList.insert(p->id(), p); // 검색 결과를 map에 저장
    }

    /* 검색 결과를 제품 검색 Dialog로 보냄 */
    for (const auto& v : qAsConst(searchList)) {
        ProductItem* p = v;
        emit sendProductToDialog(p);
    }
}

/**
* @brief 신규 제품 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int ProductManagerForm::makeId()
{
    if(productList.size( ) == 0) {
        return 1001; // id는 1001부터 시작
    } else {
        auto id = productList.lastKey();
        return ++id; // 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
    }
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
