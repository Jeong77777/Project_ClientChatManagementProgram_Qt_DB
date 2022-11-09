#include "orderitem.h"

using namespace std;

OrderItem::OrderItem(int id, QString date, int clientId, \
                     QString clientName, int productId, \
                     QString productName, int quantity, QString total)
{
    this->clientId = clientId;
    this->productId = productId;

    setText(0, QString::number(id));
    setText(1, date);
    setText(2, clientName);
    setText(3, productName);
    setText(4, QString::number(quantity));
    setText(5, total);
}

QString OrderItem::getDate() const
{
    return text(1);
}

void OrderItem::setDate(QString date)
{
    setText(1, date);
}

int OrderItem::getClinetId() const
{
    return clientId;
}

void OrderItem::setClientId(int clientId)
{
    this->clientId = clientId;
}

QString OrderItem::getClientName() const
{
    return text(2);
}

void OrderItem::setClientName(QString& clientName)
{
    setText(2, clientName);
}

int OrderItem::getProductId() const
{
    return productId;
}

void OrderItem::setProductId(int productId)
{
    this->productId = productId;
}

QString OrderItem::getProductName() const
{
    return text(3);
}

void OrderItem::setProductName(QString& productName)
{
    setText(3, productName);
}

int OrderItem::getQuantity() const
{
    return text(4).toInt();
}

void OrderItem::setQuantity(int quantity)
{
    setText(4, QString::number(quantity));
}

QString OrderItem::getTotal() const
{
    return text(5);
}

void OrderItem::setTotal(QString total)
{
    setText(5, total);
}

int OrderItem::id() const
{
    return text(0).toInt();
}
