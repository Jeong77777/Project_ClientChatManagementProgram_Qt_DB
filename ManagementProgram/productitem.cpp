#include "productitem.h"

using namespace std;

ProductItem::ProductItem(int id, QString type, QString name, int price, int stock)
{
    setText(0, QString::number(id));
    setText(1, type);
    setText(2, name);
    setText(3, QString::number(price));
    setText(4, QString::number(stock));
}

QString ProductItem::getType() const
{
    return text(1);
}

void ProductItem::setType(QString& type)
{
    setText(1, type);
}

QString ProductItem::getName() const
{
    return text(2);
}

void ProductItem::setName(QString& name)
{
    setText(2, name);
}

int ProductItem::getPrice() const
{
    return text(3).toInt();
}

void ProductItem::setPrice(int price)
{
    setText(3, QString::number(price));
}

int ProductItem::getStock() const
{
    return text(4).toInt();
}

void ProductItem::setStock(int stock)
{
    setText(4, QString::number(stock));
}

int ProductItem::id() const
{
    return text(0).toInt();
}
