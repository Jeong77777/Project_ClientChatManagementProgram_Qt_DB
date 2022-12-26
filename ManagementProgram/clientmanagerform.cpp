#include "clientmanagerform.h"
#include "ui_clientmanagerform.h"

#include <QMenu>
#include <QMessageBox>
#include <QValidator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <cassert>

/**
* @brief 생성자, split 사이즈 설정, 정규 표현식 설정, context 메뉴 설정
*/
ClientManagerForm::ClientManagerForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientManagerForm),
    menu(nullptr), removeAction(nullptr), clientModel(nullptr),
    phoneNumberRegExpValidator(nullptr)
{
    ui->setupUi(this);

    /* split 사이즈 설정 */
    QList<int> sizes;
    sizes << 170 << 400;
    ui->splitter->setSizes(sizes);

    /* 핸드폰 번호 입력 칸에 대한 정규 표현식 설정 */
    phoneNumberRegExpValidator \
            = new QRegularExpressionValidator(this);
    phoneNumberRegExpValidator\
            ->setRegularExpression(QRegularExpression("^\\d{2,3}-\\d{3,4}-\\d{4}$"));
    ui->phoneNumberLineEdit->setValidator(phoneNumberRegExpValidator);

    /* tree widget의 context 메뉴 설정 */
    // tree widget에서 고객을 삭제하는 action
    removeAction = new QAction(tr("Remove"));
    assert(connect(removeAction, SIGNAL(triggered()), SLOT(removeItem())));
    menu = new QMenu; // context 메뉴
    menu->addAction(removeAction);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    assert(connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)),\
            this, SLOT(showContextMenu(QPoint))));

    /* 검색 창에서 enter 키를 누르면 검색 버튼이 클릭되도록 connect */
    assert(connect(ui->searchLineEdit, SIGNAL(returnPressed()),
            this, SLOT(on_searchPushButton_clicked())));
}

/**
* @brief 고객 정보 데이터베이스 open
*/
void ClientManagerForm::loadData()
{
    /* 고객 정보 데이터베이스 open */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "clientConnection");
    db.setDatabaseName("clientlist.db");
    if (db.open( )) {
        // 고객 정보 테이블 생성
        QSqlQuery query(db);
        query.exec( "CREATE TABLE IF NOT EXISTS Client_list ("
                    "id          INTEGER          PRIMARY KEY, "
                    "name        VARCHAR(30)      NOT NULL,"
                    "phoneNumber VARCHAR(20)      NOT NULL,"
                    "address     VARCHAR(100)"
                    " )"
                    );

        // model로 데이터 베이스를 가져옴
        clientModel = new QSqlTableModel(this, db);
        clientModel->setTable("Client_list");
        clientModel->select();
        clientModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
        clientModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
        clientModel->setHeaderData(2, Qt::Horizontal, tr("Phone Number"));
        clientModel->setHeaderData(3, Qt::Horizontal, tr("Address"));
        ui->treeView->setModel(clientModel);
    }

    /* 채팅 서버로 고객 정보(id, 이름) 보냄 */
    for(int i = 0; i < clientModel->rowCount(); i++) {
        int id = clientModel->data(clientModel->index(i, 0)).toInt();
        std::string name = clientModel->data(clientModel->index(i, 1)).toString().toStdString();
        emit sendClientToChatServer(id, name);
    }
}

/**
* @brief 소멸자, 고객 정보 데이터베이스 저장하고 닫기
*/
ClientManagerForm::~ClientManagerForm()
{
    QSqlDatabase db = QSqlDatabase::database("clientConnection");
    if(db.isOpen()) {
        clientModel->submitAll();
        delete clientModel;
        db.commit();
        db.close();
    }

    delete phoneNumberRegExpValidator;
    phoneNumberRegExpValidator = nullptr;

    delete removeAction; removeAction = nullptr;
    delete menu; menu = nullptr;
    delete ui; ui = nullptr;
}

/**
* @brief 전체 고객 리스트 출력 버튼 슬롯, tree view에 전체 고객 리스트를 출력해 준다.
*/
void ClientManagerForm::on_showAllPushButton_clicked() const
{
    clientModel->setFilter("");  // 필터 초기화
    clientModel->select();
    ui->searchLineEdit->clear(); // 검색 창 클리어
}

/**
* @brief 검색 버튼 슬롯, tree view에 검색 결과를 출력해 준다.
*/
void ClientManagerForm::on_searchPushButton_clicked()
{
    /* 검색어 가져오기 */
    std::string str = ui->searchLineEdit->text().toStdString();
    if(!str.length()) { // 검색 창이 비어 있을 때
        QMessageBox::warning(this, tr("Search error"),
                             tr("Please enter a search term."), QMessageBox::Ok);
        return;
    }

    /* 검색 수행 */
    // 0. ID  1. 이름  2. 전화번호  3. 주소
    int i = ui->searchComboBox->currentIndex();

    switch (i) {
    case 0: clientModel->setFilter(QString("id = '%1'").arg(QString::fromStdString(str)));
        break;
    case 1: clientModel->setFilter(QString("name LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    case 2: clientModel->setFilter(QString("phoneNumber LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    case 3: clientModel->setFilter(QString("address LIKE '%%1%'").arg(QString::fromStdString(str)));
        break;
    default:
        break;
    }
    clientModel->select();

    // status bar 메시지 출력
    emit sendStatusMessage(tr("%1 search results were found")\
                           .arg(clientModel->rowCount()), 3000);

    /* 사용자가 정보를 변경해도 검색 결과가 유지되도록 ID를 이용해서 필터 재설정 */
    std::string filterStr = "id in (";
    for(int i = 0; i < clientModel->rowCount(); i++) {
        int id = clientModel->data(clientModel->index(i, 0)).toInt();
        if(i != clientModel->rowCount()-1)
            filterStr += (std::to_string(id) + ", ");
        else
            filterStr += std::to_string(id);
    }
    filterStr += ");";
    qDebug() << QString::fromStdString(filterStr);
    clientModel->setFilter(QString::fromStdString(filterStr));
}

/**
* @brief 고객 추가 버튼 슬롯, 입력 창에 입력된 정보에 따라 고객을 추가함
*/
void ClientManagerForm::on_addPushButton_clicked()
{
    /* 입력 창에 입력된 정보 가져오기 */
    std::string name, phone, address;
    int id = makeId(); // 자동으로 ID 생성
    name = ui->nameLineEdit->text().toStdString();
    phone = ui->phoneNumberLineEdit->text().toStdString();
    address = ui->addressLineEdit->text().toStdString();

    /* 입력된 정보를 DB에 추가 */
    QSqlDatabase db = QSqlDatabase::database("clientConnection");
    if(db.isOpen() && name.length() && phone.length() && address.length()) {
        QSqlQuery query(clientModel->database());
        query.prepare( "INSERT INTO Client_list "
                       "(id, name, phoneNumber, address) "
                       "VALUES "
                       "(:ID, :NAME, :PHONE, :ADDR)" );
        query.bindValue(":ID",      id);
        query.bindValue(":NAME",    QString::fromStdString(name));
        query.bindValue(":PHONE",   QString::fromStdString(phone));
        query.bindValue(":ADDR",    QString::fromStdString(address));
        query.exec();
        clientModel->select();

        cleanInputLineEdit(); // 입력 창 클리어

        // 채팅 서버로 신규 고객 정보(id, 이름) 보냄
        emit sendClientToChatServer(id, name);

        // status bar 메시지 출력
        emit sendStatusMessage(tr("Add completed (ID: %1, Name: %2)")\
                               .arg(id).arg(QString::fromStdString(name)), 3000);
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
    /* tree view에서 현재 선택된 고객의 index 가져오기 */
    QModelIndex index = ui->treeView->currentIndex();

    /* 입력 창에 입력된 정보에 따라 고객 정보를 변경 */
    if(index.isValid()) {
        // 입력 창에 입력된 정보 가져오기
        int id = clientModel->data(index.siblingAtColumn(0)).toInt();
        std::string name, phone, address;
        name = ui->nameLineEdit->text().toStdString();
        phone = ui->phoneNumberLineEdit->text().toStdString();
        address = ui->addressLineEdit->text().toStdString();

        // 입력 창에 입력된 정보에 따라 고객 정보를 변경
        if(name.length() && phone.length() && address.length()) {
            QSqlQuery query(clientModel->database());
            query.prepare("UPDATE Client_list SET name = ?, "
                          "phoneNumber = ?, address = ? WHERE id = ?");
            query.bindValue(0, QString::fromStdString(name));
            query.bindValue(1, QString::fromStdString(phone));
            query.bindValue(2, QString::fromStdString(address));
            query.bindValue(3, id);
            query.exec();
            clientModel->select();

            cleanInputLineEdit(); // 입력 창 클리어

            //채팅 서버로 변경된 고객 정보(id, 이름) 보냄
            emit sendClientToChatServer(id, name);

            // status bar 메시지 출력
            emit sendStatusMessage(tr("Modify completed (ID: %1, Name: %2)")\
                                   .arg(id).arg(QString::fromStdString(name)), 3000);
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
void ClientManagerForm::on_cleanPushButton_clicked() const
{
    cleanInputLineEdit();
}

/**
* @brief tree view의 context 메뉴 출력
* @param const QPoint &pos 우클릭한 위치
*/
void ClientManagerForm::showContextMenu(const QPoint &pos)
{
    /* tree view 위에서 우클릭한 위치에서 context menu 출력 */
    QPoint globalPos = ui->treeView->mapToGlobal(pos);
    if(ui->treeView->indexAt(pos).isValid())
        menu->exec(globalPos);
}

/**
* @brief 고객 정보 삭제
*/
void ClientManagerForm::removeItem()
{
    /* tree view에서 현재 선택된 고객의 index 가져오기 */
    QModelIndex index = ui->treeView->currentIndex();

    /* 고객 정보 삭제 */
    if(index.isValid()) {
        // 삭제된 고객 정보를 status bar에 출력해주기 위해서 id와 이름 가져오기
        int id = clientModel->data(index.siblingAtColumn(0)).toInt();
        std::string name = clientModel->data(index.siblingAtColumn(1)).toString().toStdString();

        // 고객 정보 삭제
        clientModel->removeRow(index.row());
        clientModel->select();

        // status bar 메시지 출력
        emit sendStatusMessage(tr("delete completed (ID: %1, Name: %2)")\
                               .arg(id).arg(QString::fromStdString(name)), 3000);
    }
}

/**
* @brief 주문 정보 관리 객체에서 고객ID를 가지고 고객을 검색 하기 위한 슬롯
* @Param int id 검색할 고객 id
*/
void ClientManagerForm::receiveId(const int id)
{
    /* 고객 ID를 이용하여 고객 검색 */
    QSqlQuery query(QString("select * "
                            "from Client_list "
                            "where id = '%1';").arg(id),
                    clientModel->database());
    query.exec();
    while (query.next()) {
        int id = query.value(0).toInt();
        std::string name = query.value(1).toString().toStdString();
        std::string phone = query.value(2).toString().toStdString();
        std::string address = query.value(3).toString().toStdString();

        // 검색 결과를 주문 정보 관리 객체로 보냄
        emit sendClientToOrderManager(id, name, phone, address);
    }
}

/**
* @brief 고객 검색 Dialog에서 고객을 검색 하기 위한 슬롯
* @Param std::string word 검색어(id 또는 이름)
*/
void ClientManagerForm::receiveWord(const std::string word)
{
    /* 고객 ID 또는 이름을 이용하여 고객 검색 */
    QSqlQuery query(QString("select * "
                            "from Client_list "
                            "where id = '%1' "
                            "or name LIKE '%%1%' "
                            "or phoneNumber LIKE '%%1%' "
                            "or address LIKE '%%1%';").arg(QString::fromStdString(word)),
                    clientModel->database());
    query.exec();
    while (query.next()) {
        int id = query.value(0).toInt();
        std::string name = query.value(1).toString().toStdString();
        std::string phone = query.value(2).toString().toStdString();
        std::string address = query.value(3).toString().toStdString();

        // 검색 결과를 고객 검색 Dialog로 보냄
        emit sendClientToDialog(id, name, phone, address);
    }
}

/**
* @brief 신규 고객 추가 시 ID를 자동으로 생성
* @return int 새로운 id 반환
*/
int ClientManagerForm::makeId() const
{
    // id의 최댓값 가져오기
    QSqlQuery query("select count(*), max(id) from Client_list;",
                    clientModel->database());
    query.exec();
    while (query.next()) {
        // 등록된 고객이 없을 경우, id는 10001부터 시작
        if(query.value(0).toInt() == 0)
            return 10001;
        // 등록된 고객이 있을 경우, 기존의 제일 큰 id보다 1만큼 큰 숫자를 반환
        auto id = query.value(1).toInt();
        return ++id;
    }
    return 1;
}

/**
* @brief 입력 창 클리어
*/
void ClientManagerForm::cleanInputLineEdit() const
{
    ui->idLineEdit->clear();
    ui->nameLineEdit->clear();
    ui->phoneNumberLineEdit->clear();
    ui->addressLineEdit->clear();
}

/**
* @brief tree view에서 고객을 클릭(선택)했을 때 실행되는 슬롯, 클릭된 고객의 정보를 입력 창에 표시
* @param const QModelIndex &index 선택된 고객의 index
*/
void ClientManagerForm::on_treeView_clicked(const QModelIndex &index) const
{
    /* 클릭된 고객의 정보를 가져와서 입력 창에 표시해줌 */
    std::string id = clientModel->data(index.siblingAtColumn(0)).toString().toStdString();
    std::string name = clientModel->data(index.siblingAtColumn(1)).toString().toStdString();
    std::string phoneNumber = clientModel->data(index.siblingAtColumn(2)).toString().toStdString();
    std::string address = clientModel->data(index.siblingAtColumn(3)).toString().toStdString();

    ui->idLineEdit->setText(QString::fromStdString(id));
    ui->nameLineEdit->setText(QString::fromStdString(name));
    ui->phoneNumberLineEdit->setText(QString::fromStdString(phoneNumber));
    ui->addressLineEdit->setText(QString::fromStdString(address));
}
