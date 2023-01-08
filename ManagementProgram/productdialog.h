#ifndef PRODUCTDIALOG_H
#define PRODUCTDIALOG_H

#include <QDialog>
#include <string>

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
    ProductDialog(const ProductDialog&) = delete;
    ProductDialog& operator=(const ProductDialog&) = delete;

    std::string getCurrentItem() const;           // 현재 선택된 제품ID와 이름을 반환
    void clearDialog() const;                 // 검색 결과, 입력 창 초기화

private slots:
    void on_searchPushButton_clicked();                   // 검색 버튼에 대한 슬롯
    void receiveProductInfo(const int, const std::string, const std::string, const int, const int) const; // 제품 관리 객체로부터 검색 결과를 받는 슬롯

    // tree view에서 제품을 더블클릭 하였을 때의 슬롯
    void on_treeView_doubleClicked(const QModelIndex &index);

signals:
    // 검색을 위해 제품 관리 객체로 검색어를 전달하는 시그널
    void sendWord(std::string);

private:
    Ui::ProductDialog *m_ui;
    QStandardItemModel *m_productModel;    // 검색된 제품을 저장하는 model
};

#endif // PRODUCTDIALOG_H
