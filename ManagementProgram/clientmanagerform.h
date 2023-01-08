#ifndef CLIENTMANAGERFORM_H
#define CLIENTMANAGERFORM_H

#include <QWidget>
#include <string>

class QMenu;
class QSqlTableModel;
class QAction;
class QRegularExpressionValidator;

namespace Ui {
class ClientManagerForm;
}

/**
* @brief 고객 정보를 관리하는 클래스
*/
class ClientManagerForm : public QWidget
{
    Q_OBJECT

public:
    explicit ClientManagerForm(QWidget *parent = nullptr);
    ~ClientManagerForm();
    ClientManagerForm(const ClientManagerForm&) = delete;
    ClientManagerForm& operator=(const ClientManagerForm&) = delete;

    void loadData(); // 저장되어 있는 고객 리스트 불러오기

private slots:
    void on_showAllPushButton_clicked() const; // 전체 고객 리스트 출력 버튼 슬롯
    void on_searchPushButton_clicked();  // 검색 버튼 슬롯
    void on_addPushButton_clicked();     // 고객 추가 버튼 슬롯
    void on_modifyPushButton_clicked();  // 고객 정보 변경 버튼 슬롯
    void on_cleanPushButton_clicked() const;   // 입력 창 클리어 버튼 슬롯
    // tree view에서 고객을 클릭(선택)했을 때의 슬롯
    void on_treeView_clicked(const QModelIndex &index) const;

    void showContextMenu(const QPoint &); // tree view의 context 메뉴 출력
    void removeItem();                    // 고객 정보 삭제
    // 주문 정보 관리 객체에서 고객ID를 가지고 고객을 검색 하기 위한 슬롯
    void receiveId(const int);
    // 고객 검색 Dialog에서 고객을 검색 하기 위한 슬롯
    void receiveWord(const std::string);

signals:
    // 고객 검색 Dialog로 검색된 고객 정보를 보내주는 시그널
    void sendClientToDialog(int, std::string, std::string, std::string);
    // 주문 정보 관리 객체로 검색된 고객 정보를 보내주는 시그널
    void sendClientToOrderManager(int, std::string, std::string, std::string);
    // 채팅 서버로 고객 정보를 보내주는 시그널
    void sendClientToChatServer(int, std::string);
    // status bar에 표시될 메시지를 보내주는 시그널
    void sendStatusMessage(QString, int);

private:
    int makeId() const;              // Id를 자동으로 생성
    void cleanInputLineEdit() const; // 입력 창 클리어

    Ui::ClientManagerForm *m_ui;      // ui
    QMenu* m_menu;                    // tree widget context 메뉴
    QAction* m_removeAction;
    QSqlTableModel* m_clientModel;    // 고객 정보 데이터베이스 모델
    QRegularExpressionValidator* m_phoneNumberRegExpValidator;
};

#endif // CLIENTMANAGERFORM_H
