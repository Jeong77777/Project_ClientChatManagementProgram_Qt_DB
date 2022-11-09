#ifndef PRODUCTITEM_H
#define PRODUCTITEM_H

#include <QTreeWidgetItem>

/**
* @brief 제품 정보를 저장하는 tree widget item 클래스
*/
class ProductItem : public QTreeWidgetItem
{
public:
    explicit ProductItem(int id = 0, QString = "", \
                         QString = "", int = 0, int = 0);

    QString getType() const;
    void setType(QString&);
    QString getName() const;
    void setName(QString&);
    int getStock() const;
    void setStock(int);
    int getPrice() const;
    void setPrice(int);
    int id() const;
};

#endif // PRODUCTITEM_H
