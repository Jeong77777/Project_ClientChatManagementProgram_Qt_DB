#include "productdialog.h"
#include "ui_productdialog.h"

/**
* @brief 생성자, dialog 초기화
*/
ProductDialog::ProductDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProductDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Product Info"));
    setWindowModality(Qt::ApplicationModal);

    connect(ui->lineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked()));

    ui->searchPushButton->setFocus();
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
* @param c 검색된 제품
*/
void ProductDialog::receiveProductInfo(ProductItem * p)
{
//    /* 검색 결과를 tree widget에 추가 */
//    ProductItem* product = new ProductItem(p->id(), p->getType(), \
//                                           p->getName(), p->getPrice(), \
//                                           p->getStock());
//    ui->treeWidget->addTopLevelItem(product);
}

/**
* @brief 현재 선택된 제품 item을 반환
* @return 현재 선택된 제품 item
*/
QTreeWidgetItem* ProductDialog::getCurrentItem()
{
    return ui->treeWidget->currentItem();
}

/**
* @brief 검색 결과, 입력 창 초기화
*/
void ProductDialog::clearDialog()
{
    ui->treeWidget->clear();
    ui->lineEdit->clear();
    ui->searchPushButton->setFocus();
}

/**
* @brief 검색 버튼에 대한 슬롯, 검색 실행
*/
void ProductDialog::on_searchPushButton_clicked()
{
    /* 검색을 위해 제품 관리 객체로 검색어를 전달하는 시그널 emit */
    ui->treeWidget->clear();
    emit sendWord(ui->lineEdit->text());
}

