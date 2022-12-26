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
    m_ui(new Ui::ProductDialog), m_productModel(nullptr)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Product Info"));
    setWindowModality(Qt::ApplicationModal);

    assert(connect(m_ui->lineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked())));

    m_ui->searchPushButton->setDefault(true);

    /* 검색된 제품을 저장하는 model 초기화 */
    m_productModel = new QStandardItemModel(0, 4);
    m_productModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    m_productModel->setHeaderData(1, Qt::Horizontal, tr("Type"));
    m_productModel->setHeaderData(2, Qt::Horizontal, tr("Name"));
    m_productModel->setHeaderData(3, Qt::Horizontal, tr("Unit Price"));
    m_productModel->setHeaderData(4, Qt::Horizontal, tr("Quantities in stock"));
    m_ui->treeView->setModel(m_productModel);
}

/**
* @brief 소멸자
*/
ProductDialog::~ProductDialog()
{
    int row = m_productModel->rowCount();
    for(int i = 0; i < row; i++) {
        QList<QStandardItem *> itmes = m_productModel->takeRow(0);
        for(const auto& item : itmes)
            delete item;
    }

    delete m_productModel; m_productModel = nullptr;
    delete m_ui; m_ui = nullptr;
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

    m_productModel->appendRow(items);
}

/**
* @brief 현재 선택된 제품ID와 이름을 반환
* @return 현재 선택된 제품ID(이름)
*/
std::string ProductDialog::getCurrentItem() const
{
    QModelIndex index = m_ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = m_productModel->data(index.siblingAtColumn(0)).toInt();
        std::string name = m_productModel->data(index.siblingAtColumn(2)).toString().toStdString();
        return std::to_string(id)+" ("+name+")";
    }
    return "";
}

/**
* @brief 검색 결과, 입력 창 초기화
*/
void ProductDialog::clearDialog() const
{
    int row = m_productModel->rowCount();
    for(int i = 0; i < row; i++) {
        QList<QStandardItem *> itmes = m_productModel->takeRow(0);
        for(const auto& item : itmes)
            delete item;
    }
    m_ui->lineEdit->clear();
}

/**
* @brief 검색 버튼에 대한 슬롯, 검색 실행
*/
void ProductDialog::on_searchPushButton_clicked()
{
    /* 검색을 위해 제품 관리 객체로 검색어를 전달하는 시그널 emit */
    int row = m_productModel->rowCount();
    for(int i = 0; i < row; i++) {
        QList<QStandardItem *> itmes = m_productModel->takeRow(0);
        for(const auto& item : itmes)
            delete item;
    }
    emit sendWord(m_ui->lineEdit->text().toStdString());
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

