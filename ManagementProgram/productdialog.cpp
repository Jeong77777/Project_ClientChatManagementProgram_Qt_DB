#include "productdialog.h"
#include "ui_productdialog.h"

#include <QStandardItemModel>
#include <cassert>
#include <vector>

/**
* @brief 생성자, dialog 초기화
*/
ProductDialog::ProductDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProductDialog), productModel(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Product Info"));
    setWindowModality(Qt::ApplicationModal);

    assert(connect(ui->lineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked())));

    ui->searchPushButton->setDefault(true);

    /* 검색된 제품을 저장하는 model 초기화 */
    productModel = new QStandardItemModel(0, 4);
    productModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    productModel->setHeaderData(1, Qt::Horizontal, tr("Type"));
    productModel->setHeaderData(2, Qt::Horizontal, tr("Name"));
    productModel->setHeaderData(3, Qt::Horizontal, tr("Unit Price"));
    productModel->setHeaderData(4, Qt::Horizontal, tr("Quantities in stock"));
    ui->treeView->setModel(productModel);
}

/**
* @brief 소멸자
*/
ProductDialog::~ProductDialog()
{
    delete ui;
}

/**
* @brief 제품 관리 객체로부터 검색 결과를 받는 슬롯
* @param int id 검색된 제품의 ID
* @param std::string type 제품종류
* @param std::string name 이름
* @param int price 가격
* @param int stock 재고수량
*/
void ProductDialog::receiveProductInfo(int id, std::string type, \
                                       std::string name, int price, int stock) const
{
    /* 검색 결과를 model에 추가 */
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

    productModel->appendRow(items);
}

/**
* @brief 현재 선택된 제품ID와 이름을 반환
* @return 현재 선택된 제품ID(이름)
*/
std::string ProductDialog::getCurrentItem() const
{
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = productModel->data(index.siblingAtColumn(0)).toInt();
        std::string name = productModel->data(index.siblingAtColumn(2)).toString().toStdString();
        return std::to_string(id)+" ("+name+")";
    }
    return "";
}

/**
* @brief 검색 결과, 입력 창 초기화
*/
void ProductDialog::clearDialog() const
{
    productModel->removeRows(0, productModel->rowCount());
    ui->lineEdit->clear();
    ui->searchPushButton->setFocus();
}

/**
* @brief 검색 버튼에 대한 슬롯, 검색 실행
*/
void ProductDialog::on_searchPushButton_clicked()
{
    /* 검색을 위해 제품 관리 객체로 검색어를 전달하는 시그널 emit */
    productModel->removeRows(0, productModel->rowCount());
    emit sendWord(ui->lineEdit->text().toStdString());
}

/**
* @brief tree view에서 제품을 더블클릭 하였을 때의 슬롯
*/
void ProductDialog::on_treeView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    // accept를 하면서 창을 닫는다
    accept();
}

