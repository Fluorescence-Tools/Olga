#ifndef DOMAINTABLEMODEL_H
#define DOMAINTABLEMODEL_H

#include <memory>

#include <QAbstractTableModel>
#include <QVector>
#include <QJsonArray>

#include "AV/MolecularSystemDomain.h"

class DomainTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit DomainTableModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

    bool load(const QJsonArray &domainsArr);
    QJsonArray jsonArray() const;
    const std::shared_ptr<MolecularSystemDomain> domain(int i) const;
    QVector<QString> names() const;
Q_SIGNALS:

public Q_SLOTS:

private:
    QVector<std::shared_ptr<MolecularSystemDomain>> _domains;
};

#endif // DOMAINTABLEMODEL_H
