#ifndef CLIENTITEM_H
#define CLIENTITEM_H

#include <QTreeWidgetItem>

/**
* @brief 고객 정보를 저장하는 tree widget item 클래스
*/
class ClientItem : public QTreeWidgetItem
{
public:
    explicit ClientItem(int id = 0, QString = "", QString = "", QString = "");

    QString getName() const;
    void setName(QString&);
    QString getPhoneNumber() const;
    void setPhoneNumber(QString&);
    QString getAddress() const;
    void setAddress(QString&);
    int id() const;
};

#endif // CLIENTITEM_H
