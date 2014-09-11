#ifndef POSITIONTABLEMODEL_H
#define POSITIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QJsonArray>

#include "AV/Position.h"

class PositionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PositionTableModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

    bool load(const QJsonObject &positionsListObject);
    QJsonObject jsonArray() const;
    const Position& position(int i) const;
    const std::vector<Position> &positions() const;

signals:

public slots:

private:
    std::vector<Position> _positions;
};

#endif // POSITIONTABLEMODEL_H
