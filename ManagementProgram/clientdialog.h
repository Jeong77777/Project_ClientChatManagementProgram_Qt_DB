#ifndef CLIENTDIALOG_H
#define CLIENTDIALOG_H

#include <QDialog>

class QStandardItemModel;

namespace Ui {
class ClientDialog;
}

/**
* @brief 고객 검색 기능을 제공하는 dialog
*/
class ClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClientDialog(QWidget *parent = nullptr);
    ~ClientDialog();

    QString getCurrentItem();          // 현재 선택된 고객ID와 이름을 반환
    void clearDialog();                // 검색 결과, 입력 창 초기화

private slots:
    void on_searchPushButton_clicked();                     // 검색 버튼에 대한 슬롯
    void receiveClientInfo(int, QString, QString, QString); // 고객 관리 객체로부터 검색 결과를 받는 슬롯

    // tree view에서 고객을 더블클릭 하였을 때의 슬롯
    void on_treeView_doubleClicked(const QModelIndex &index);

signals:
    // 검색을 위해 고객 관리 객체로 검색어를 전달하는 시그널
    void sendWord(QString);

private:
    Ui::ClientDialog *ui;
    QStandardItemModel *clientModel;    // 검색된 고객을 저장하는 model
};

#endif // CLIENTDIALOG_H
