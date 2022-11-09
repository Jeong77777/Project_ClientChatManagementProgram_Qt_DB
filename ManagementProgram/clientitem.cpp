#include "clientitem.h"

using namespace std;

ClientItem::ClientItem(int id, QString name, QString phoneNumber, QString address)
{
    setText(0, QString::number(id));
    setText(1, name);
    setText(2, phoneNumber);
    setText(3, address);
}

QString ClientItem::getName() const
{
    return text(1);
}

void ClientItem::setName(QString& name)
{
    setText(1, name);
}

QString ClientItem::getPhoneNumber() const
{
    return text(2);
}

void ClientItem::setPhoneNumber(QString& phoneNumber)
{
    setText(2, phoneNumber);
}

QString ClientItem::getAddress() const
{
    return text(3);
}

void ClientItem::setAddress(QString& address)
{
    setText(3, address);
}

int ClientItem::id() const
{
    return text(0).toInt();
}
