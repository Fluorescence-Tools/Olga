#ifndef DISTANCETABLEMODEL_H
#define DISTANCETABLEMODEL_H

#include <QVector>
#include <QAbstractTableModel>

#include "Distance.h"

class DistanceTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit DistanceTableModel(QObject *parent = 0);
    int rowCount(const QModelIndex& parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

    bool load(const QJsonObject &distancesListObject);
    QJsonObject jsonArray() const;
    const std::vector<Distance>& distances() const
    {
        return _distances;
    }
signals:

public slots:
private:
    std::vector<Distance> _distances;
};

#endif // DISTANCETABLEMODEL_H
