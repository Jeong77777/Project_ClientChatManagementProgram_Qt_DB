#ifndef ORDERITEM_H
#define ORDERITEM_H

#include <QTreeWidgetItem>

/**
* @brief 주문 정보를 저장하는 tree widget item 클래스
*/
class OrderItem : public QTreeWidgetItem
{
public:
    explicit OrderItem(int id = 0, QString date="0000-00-00", \
                       int clientId = 0, QString clientName = "", \
                       int productId = 0, QString productName = "", \
                       int quantity = 0, QString total = "0");

    int getClinetId() const;
    void setClientId(int);
    QString getDate() const;
    void setDate(QString);
    QString getClientName() const;
    void setClientName(QString&);
    int getProductId() const;
    void setProductId(int);
    QString getProductName() const;
    void setProductName(QString&);
    int getQuantity() const;
    void setQuantity(int);
    QString getTotal() const;
    void setTotal(QString);
    int id() const;

private:
    int clientId;
    int productId;
};

#endif // ORDERITEM_H
