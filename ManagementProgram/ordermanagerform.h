#ifndef ORDERMANAGERFORM_H
#define ORDERMANAGERFORM_H

#include <QWidget>
#include <string>

class QMenu;
class ClientDialog;
class ProductDialog;
class QSqlTableModel;
class QStandardItemModel;

namespace Ui {
class OrderManagerForm;
}

/**
* @brief 주문 정보를 관리하는 클래스
*/
class OrderManagerForm : public QWidget
{
    Q_OBJECT

public:
    explicit OrderManagerForm(QWidget *parent = nullptr,
                              ClientDialog *clientDialog = nullptr,
                              ProductDialog *productDialog = nullptr);
    ~OrderManagerForm();

    void loadData();  // 저장되어 있는 주문 리스트 불러오기

private slots:
    void on_showAllPushButton_clicked(); // 전체 주문 리스트 출력 버튼 슬롯
    // 검색 항목 선택 콤보 박스에서 선택된 것이 변경되었을 때의 슬롯
    void on_searchComboBox_currentIndexChanged(int index);
    void on_searchPushButton_clicked();       // 검색 버튼 슬롯
    void on_inputClientPushButton_clicked();  // 고객 검색, 입력 버튼 슬롯
    void on_inputProductPushButton_clicked(); // 제품 검색, 입력 버튼 슬롯
    void on_addPushButton_clicked();          // 주문 추가 버튼 슬롯
    void on_modifyPushButton_clicked();       // 주문 정보 변경 버튼 슬롯
    void on_cleanPushButton_clicked();        // 입력 창 클리어 버튼 슬롯
    // tree view에서 주문을 클릭(선택)했을 때의 슬롯
    void on_treeView_clicked(const QModelIndex &index);

    void showContextMenu(const QPoint &); // tree view의 context 메뉴 출력
    void removeItem();                    // 주문 정보 삭제
    // 고객 정보 관리 객체로부터 고객 정보를 받기 위한 슬롯
    void receiveClientInfo(int, std::string, std::string, std::string);
    // 제품 정보 관리 객체로부터 제품 정보를 받기 위한 슬롯
    void receiveProductInfo(int, QString, QString, int, int);

signals:
    // 고객 정보 관리 객체로 가져올 고객 정보에 대한 ID를 보내는 시그널
    void sendClientId(int);
    // 제품 정보 관리 객체로 가져올 제품 정보에 대한 ID를 보내는 시그널
    void sendProductId(int);
    // status bar에 표시될 메시지를 보내주는 시그널
    void sendStatusMessage(QString, int);
    // 제품 정보 관리 객체를 통해서 재고량을 변경하기 위한 시그널
    void sendStock(int, int);

private:
    int makeId();              // Id를 자동으로 생성
    void cleanInputLineEdit(); // 입력 창 클리어

    ClientDialog *clientDialog;   // 고객을 검색, 입력하기 위한 다이얼로그
    ProductDialog *productDialog; // 제품을 검색, 입력하기 위한 다이얼로그

    Ui::OrderManagerForm *ui;       // ui
    QMenu* menu;                    // tree view context 메뉴
    QSqlTableModel* orderModel;     // 주문 정보 데이터베이스 모델

    QStandardItemModel *clientModel;    // 고객 정보 관리 객체로부터 가져온 고객을 저장하는 model
    QStandardItemModel *productModel;   // 제품 정보 관리 객체로부터 가져온 제품을 저장하는 model
    bool searchedClientFlag;            // 고객 검색 결과 (결과가 있으면 true, 없으면 false)
    bool searchedProductFlag;           // 제품 검색 결과 (결과가 있으면 true, 없으면 false)
};

#endif // ORDERMANAGERFORM_H
