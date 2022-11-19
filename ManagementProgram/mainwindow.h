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

/**
 * @brief MainWindow 클래스
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionClient_triggered();
    void on_actionProduct_triggered();
    void on_actionOrder_triggered();
    void on_actionChat_triggered();

private:
    Ui::MainWindow *ui;
    ClientManagerForm *clientForm;      // 고객 정보 관리
    ProductManagerForm *productForm;    // 제품 정보 관리
    OrderManagerForm *orderForm;        // 주문 정보 관리
    ChatServerForm *chatForm;           // 채팅 서버

};
#endif // MAINWINDOW_H
