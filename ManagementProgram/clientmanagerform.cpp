#include "clientmanagerform.h"
#include "ui_clientmanagerform.h"
//#include "clientitem.h"

#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QValidator>
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>

/**
* @brief 생성자, split 사이즈 설정, 정규 표현식 설정, context 메뉴 설정
*/
ClientManagerForm::ClientManagerForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientManagerForm)
{
    ui->setupUi(this);    

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 170 << 400;
    ui->splitter->setSizes(sizes);

    /* 핸드폰 번호 입력 칸에 대한 정규 표현식 설정 */
    QRegularExpressionValidator* phoneNumberRegExpValidator \
            = new QRegularExpressionValidator(this);
    phoneNumberRegExpValidator\
            ->setRegularExpression(QRegularExpression("^\\d{2,3}-\\d{3,4}-\\d{4}$"));
    ui->phoneNumberLineEdit->setValidator(phoneNumberRegExpValidator);

    /* tree view의 context 메뉴 설정 */
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
}

/**
* @brief clientlist.txt 파일을 열어서 저장된 고객 리스트를 가져옴
*/
void ClientManagerForm::loadData()
{
    clientModel = new QSqlTableModel(this);
    clientModel->setTable("Client_list");
    clientModel->select();
    clientModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    clientModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    clientModel->setHeaderData(2, Qt::Horizontal, tr("Phone Number"));
    clientModel->setHeaderData(3, Qt::Horizontal, tr("Address"));
    ui->treeView->setModel(clientModel);

    for(int i = 0; i < clientModel->rowCount(); i++) {
        int id = clientModel->data(clientModel->index(i, 0)).toInt();
        QString name = clientModel->data(clientModel->index(i, 1)).toString();
        // 채팅 서버로 고객 정보(id, 이름) 보냄
        emit sendClientToChatServer(id, name);
    }
}

/**
* @brief 소멸자, 고객 리스트를 clientlist.txt에 저장
*/
ClientManagerForm::~ClientManagerForm()
{

}

/**
* @brief 전체 고객 리스트 출력 버튼 슬롯, tree widget에 전체 고객 리스트를 출력해 준다.
*/
void ClientManagerForm::on_showAllPushButton_clicked()
{
    clientModel->setFilter("");
    clientModel->select();


    ui->searchLineEdit->clear(); // 검색 창 클리어
}

/**
* @brief 검색 버튼 슬롯, tree widget에 검색 결과를 출력해 준다.
*/
void ClientManagerForm::on_searchPushButton_clicked()
{
    /* 검색어 가져오기 */
    QString str = ui->searchLineEdit->text();
    if(!str.length()) { // 검색 창이 비어 있을 때
        QMessageBox::warning(this, tr("Search error"),
                             tr("Please enter a search term."), QMessageBox::Ok);
        return;
    }

    int i = ui->searchComboBox->currentIndex(); //무엇으로 검색할지 콤보박스의 인덱스를 가져옴

    switch (i) {
    case 0: clientModel->setFilter(QString("c_id = '%1'").arg(str));
        break;
    case 1: clientModel->setFilter(QString("c_name LIKE '%%1%'").arg(str));
        break;
    case 2: clientModel->setFilter(QString("c_phone LIKE '%%1%'").arg(str));
        break;
    case 3: clientModel->setFilter(QString("c_addr LIKE '%%1%'").arg(str));
        break;
    default:
        break;
    }
    clientModel->select();
    emit sendStatusMessage(tr("%1 search results were found").arg(clientModel->rowCount()), 3000);


    QString filterStr = "c_id in (";
    for(int i = 0; i < clientModel->rowCount(); i++) {
        int id = clientModel->data(clientModel->index(i, 0)).toInt();
        if(i != clientModel->rowCount()-1)
            filterStr += QString("%1, ").arg(id);
        else
            filterStr += QString("%1);").arg(id);
    }
    if(clientModel->rowCount()==0) filterStr = "";
    qDebug() << filterStr;
    clientModel->setFilter(filterStr);
}


/**
* @brief 고객 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 고객을 추가함
*/
void ClientManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    QString name, phone, address;
    int id = makeId(); // 자동으로 ID 생성
    name = ui->nameLineEdit->text();
    phone = ui->phoneNumberLineEdit->text();
    address = ui->addressLineEdit->text();

    /* 입력된 정보로 db에 추가 */
    if(name.length() && phone.length() && address.length()) {
        QSqlQuery qry;

        qry.prepare( "INSERT INTO Client_list "
                     "(c_id, c_name, c_phone, c_addr) "
                     "VALUES "
                     "(:ID, :NAME, :PHONE, :ADDR)" );

        qry.bindValue(":ID",        id);
        qry.bindValue(":NAME",      name);
        qry.bindValue(":PHONE",     phone);
        qry.bindValue(":ADDR",     address);
        if( !qry.exec() )
          qDebug() << qry.lastError();

        clientModel->select();

        cleanInputLineEdit(); // 입력 창 클리어

        // 채팅 서버로 신규 고객 정보(id, 이름) 보냄
        emit sendClientToChatServer(id, name); // 채팅 서버로

        emit sendStatusMessage(tr("Add completed"), 3000);
    }
    else { // 비어있는 입력 창이 있을 때
        QMessageBox::warning(this, tr("Add error"),
                             QString(tr("Some items have not been entered.")),\
                             QMessageBox::Ok);
    }
}

/**
* @brief 고객 정보 변경 버튼 슬롯, 입력 창에 입력된 정보에 따라 고객 정보를 변경함
*/
void ClientManagerForm::on_modifyPushButton_clicked()
{    
    QModelIndex index = ui->treeView->currentIndex();
    if(index.isValid()) {
        int id = clientModel->data(index.siblingAtColumn(0)).toInt();
        QString name, number, address;
        name = ui->nameLineEdit->text();
        number = ui->phoneNumberLineEdit->text();
        address = ui->addressLineEdit->text();

        // 입력 창에 입력된 정보에 따라 고객 정보를 변경
        if(name.length() && number.length() && address.length()) {
            QSqlQuery query(clientModel->database());
            query.prepare("UPDATE Client_list SET c_name = ?, c_phone = ?, c_addr = ? WHERE c_id = ?");
            query.bindValue(0, name);
            query.bindValue(1, number);
            query.bindValue(2, address);
            query.bindValue(3, id);
            query.exec();
            clientModel->select();

            //채팅 서버로 변경된 고객 정보(id, 이름) 보냄
            emit sendClientToChatServer(id, name);

            emit sendStatusMessage(tr("Modify completed"), 3000);
        }
        else { // 비어있는 입력 창이 있을 때
            QMessageBox::warning(this, tr("Modify error"),\
                                 QString(tr("Some items have not been entered.")),\
                                 QMessageBox::Ok);
        }
    }
}

/**
* @brief 입력 창 클리어 버튼 슬롯, 입력 창을 클리어 하는 함수 호출
*/
void ClientManagerForm::on_cleanPushButton_clicked()
{
    cleanInputLineEdit();
}

/**
* @brief tree widget에서 제품을 클릭(선택)했을 때 실행되는 슬롯, 클릭된 제품의 정보를 입력 창에 표시
* @Param QTreeWidgetItem *item 클릭된 item
* @Param int column 클릭된 item의 열
*/
//void ClientManagerForm::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
//{
//    Q_UNUSED(column);

//    /* 클릭된 고객의 정보를 입력 창에 표시해줌 */
//    ui->idLineEdit->setText(item->text(0));
//    ui->nameLineEdit->setText(item->text(1));
//    ui->phoneNumberLineEdit->setText(item->text(2));
//    ui->addressLineEdit->setText(item->text(3));
//}

/**
* @brief tree widget의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void ClientManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree widget 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeView->mapToGlobal(pos);
    if(ui->treeView->indexAt(pos).isValid())
        menu->exec(globalPos);
}

/**
* @brief 고객 정보 삭제
*/
void ClientManagerForm::removeItem()
{
    QModelIndex index = ui->treeView->currentIndex();
    if(index.isValid()) {
        clientModel->removeRow(index.row());
        clientModel->select();
        emit sendStatusMessage(tr("delete completed"), 3000);
    }
}

/**
* @brief 주문 정보 관리 객체에서 고객ID를 가지고 고객을 검색 하기 위한 슬롯
* @Param int id 검색할 고객 id
*/
void ClientManagerForm::receiveId(int id)
{
    QSqlQueryModel model;
    model.setQuery(QString("select * "
                           "from Client_list "
                           "where c_id = '%1';").arg(id));

    if(model.rowCount() != 0) {
        int id = model.data(model.index(0, 0)).toInt();
        QString name = model.data(model.index(0, 1)).toString();
        QString phone = model.data(model.index(0, 2)).toString();
        QString address = model.data(model.index(0, 3)).toString();

        QTreeWidgetItem* item  = new QTreeWidgetItem;
        item->setText(0, QString::number(id));
        item->setText(1, name);
        item->setText(2, phone);
        item->setText(3, address);

        //검색 결과를 주문 정보 관리 객체로 보냄
        emit sendClientToOrderManager(item);
    }
}

/**
* @brief 고객 검색 Dialog에서 고객을 검색 하기 위한 슬롯
* @Param QString word 검색어(id 또는 이름)
*/
void ClientManagerForm::receiveWord(QString word)
{
    QSqlQueryModel model;
    model.setQuery(QString("select * "
                   "from Client_list "
                   "where c_id = '%1' "
                   "or c_name LIKE '%%1%' "
                   "or c_phone LIKE '%%1%' "
                   "or c_addr LIKE '%%1%';").arg(word));

    for(int i = 0; i < model.rowCount(); i++) {
        int id = model.data(model.index(i, 0)).toInt();
        QString name = model.data(model.index(i, 1)).toString();
        QString phone = model.data(model.index(i, 2)).toString();
        QString address = model.data(model.index(i, 3)).toString();
        qDebug() << id << name;
        QTreeWidgetItem* item  = new QTreeWidgetItem;
        item->setText(0, QString::number(id));
        item->setText(1, name);
        item->setText(2, phone);
        item->setText(3, address);

        emit sendClientToDialog(item);
    }
}

/**
* @brief 신규 고객 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int ClientManagerForm::makeId()
{
    QSqlQueryModel model;
    model.setQuery("select count(*), max(c_id) from Client_list;");

    if(model.data(model.index(0,0)).toInt() == 0) {
        return 10001;
    } else {
        auto id = model.data(model.index(0,1)).toInt();
        return ++id;
    }
}

/**
* @brief 입력 창 클리어
*/
void ClientManagerForm::cleanInputLineEdit()
{
    ui->idLineEdit->clear();
    ui->nameLineEdit->clear();
    ui->phoneNumberLineEdit->clear();
    ui->addressLineEdit->clear();
}

void ClientManagerForm::on_treeView_clicked(const QModelIndex &index)
{
    QString id = clientModel->data(index.siblingAtColumn(0)).toString();
    QString name = clientModel->data(index.siblingAtColumn(1)).toString();
    QString phoneNumber = clientModel->data(index.siblingAtColumn(2)).toString();
    QString address = clientModel->data(index.siblingAtColumn(3)).toString();

    ui->idLineEdit->setText(id);
    ui->nameLineEdit->setText(name);
    ui->phoneNumberLineEdit->setText(phoneNumber);
    ui->addressLineEdit->setText(address);
}

