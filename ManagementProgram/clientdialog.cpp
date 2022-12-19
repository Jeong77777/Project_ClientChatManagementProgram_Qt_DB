#include "clientdialog.h"
#include "ui_clientdialog.h"

#include <QStandardItemModel>
#include <cassert>

/**
* @brief 생성자, dialog 초기화
*/
ClientDialog::ClientDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClientDialog), clientModel(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Client Info"));
    setWindowModality(Qt::ApplicationModal);

    assert(connect(ui->lineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked())));

    ui->searchPushButton->setDefault(true);

    /* 검색된 고객을 저장하는 model 초기화 */
    clientModel = new QStandardItemModel(0, 4);
    clientModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    clientModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    clientModel->setHeaderData(2, Qt::Horizontal, tr("Phone Number"));
    clientModel->setHeaderData(3, Qt::Horizontal, tr("Address"));
    ui->treeView->setModel(clientModel);
}

/**
* @brief 소멸자
*/
ClientDialog::~ClientDialog()
{
    delete ui;
}

/**
* @brief 고객 관리 객체로부터 검색 결과를 받는 슬롯
* @param int id 검색된 고객의 ID
* @param QString name 이름
* @param QString phone 전화번호
* @param QString address 주소
*/
void ClientDialog::receiveClientInfo(int id, QString name, \
                                     QString phone, QString address)
{
    /* 검색 결과를 model에 추가 */
    QStringList strings;
    strings << QString::number(id) << name << phone << address;

    QList<QStandardItem *> items;
    for (int i = 0; i < 4; ++i) {
        items.append(new QStandardItem(strings.at(i)));
    }

    clientModel->appendRow(items);
}

/**
* @brief 현재 선택된 고객ID와 이름을 반환
* @return 현재 선택된 고객ID(이름)
*/
QString ClientDialog::getCurrentItem()
{
    QModelIndex index = ui->treeView->currentIndex();

    if(index.isValid()) {
        int id = clientModel->data(index.siblingAtColumn(0)).toInt();
        QString name = clientModel->data(index.siblingAtColumn(1)).toString();
        return QString::number(id)+" ("+name+")";
    }
    else
        return "";
}

/**
* @brief 검색 결과, 입력 창 초기화
*/
void ClientDialog::clearDialog()
{
    clientModel->removeRows(0, clientModel->rowCount());
    ui->lineEdit->clear();
}

/**
* @brief 검색 버튼에 대한 슬롯, 검색 실행
*/
void ClientDialog::on_searchPushButton_clicked()
{
    /* 검색을 위해 고객 관리 객체로 검색어를 전달하는 시그널 emit */
    clientModel->removeRows(0, clientModel->rowCount());
    emit sendWord(ui->lineEdit->text());
}

/**
* @brief tree view에서 고객을 더블클릭 하였을 때의 슬롯
*/
void ClientDialog::on_treeView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    // accept를 하면서 창을 닫는다
    accept();
}
