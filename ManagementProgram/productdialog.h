#ifndef PRODUCTDIALOG_H
#define PRODUCTDIALOG_H

#include <QDialog>

class ProductItem;
class QStandardItemModel;

namespace Ui {
class ProductDialog;
}

/**
* @brief 제품 검색 기능을 제공하는 dialog
*/
class ProductDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductDialog(QWidget *parent = nullptr);
    ~ProductDialog();

    QString getCurrentItem(); // 현재 선택된 제품 item을 반환
    void clearDialog();                // 검색 결과, 입력 창 초기화

private slots:
    void on_searchPushButton_clicked();     // 검색 버튼에 대한 슬롯
    void receiveProductInfo(int,QString,QString,int,int); // 제품 관리 객체로부터 검색 결과를 받는 슬롯



    void on_treeView_doubleClicked(const QModelIndex &index);

signals:
    // 검색을 위해 제품 관리 객체로 검색어를 전달하는 시그널
    void sendWord(QString);

private:
    Ui::ProductDialog *ui;
    QStandardItemModel *productModel;
};

#endif // PRODUCTDIALOG_H
