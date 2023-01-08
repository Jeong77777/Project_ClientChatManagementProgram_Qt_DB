#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ClientManagerForm;
class ProductManagerForm;
class OrderManagerForm;
class ChatServerForm;
class ClientDialog;
class ProductDialog;

/**
 * @brief MainWindow 클래스
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

private slots:
    void on_actionClient_triggered() const;
    void on_actionProduct_triggered() const;
    void on_actionOrder_triggered() const;
    void on_actionChat_triggered() const;

private:
    Ui::MainWindow *m_ui;
    ClientManagerForm *m_clientForm;      // 고객 정보 관리
    ProductManagerForm *m_productForm;    // 제품 정보 관리
    OrderManagerForm *m_orderForm;        // 주문 정보 관리
    ChatServerForm *m_chatForm;           // 채팅 서버

    /* 고객, 제품 검색 기능을 제공하는 dialog */
    ClientDialog *m_clientDialog;
    ProductDialog *m_productDialog;

};
#endif // MAINWINDOW_H
