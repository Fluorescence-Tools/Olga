#include "PositionTableModel.h"



PositionTableModel::PositionTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

int PositionTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _positions.size();
}

int PositionTableModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 6;
}

QVariant PositionTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case 0:
            return QString::fromStdString(_positions.at(index.row()).name());
        case 1:
            return QString::fromStdString(_positions.at(index.row()).chainIdentifier());
        case 2:
            return _positions.at(index.row()).residueSeqNumber();
        case 3:
            return QString::fromStdString(_positions.at(index.row()).residueName());
        case 4:
            return QString::fromStdString(_positions.at(index.row()).atomName());
        case 5:
            return QString::fromStdString(_positions.at(index.row()).simulationType());
        }

        return QString("Row%1, Column%2")
                .arg(index.row() + 1)
                .arg(index.column() +1);
    }
    return QVariant();
}

QVariant PositionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Vertical)
        {
            return section+1;
        }
        else if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Position name");
            case 1:
                return QString("Chain");
            case 2:
                return QString("Res.#");
            case 3:
                return QString("Res.name");
            case 4:
                return QString("Atom");
            case 5:
                return QString("Sim.type");
            }
        }
    }
    return QVariant();
}

bool PositionTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        switch(index.column())
        {
        case 0:
            _positions[index.row()].setName(value.toString().toStdString());
            break;
        case 1:
            _positions[index.row()].setChainIdentifier(value.toString().toStdString());
            break;
        case 2:
            _positions[index.row()].setResidueSeqNumber(value.toInt());
            break;
        case 3:
            _positions[index.row()].setResidueName(value.toString().toStdString());
            break;
        case 4:
            _positions[index.row()].setAtomName(value.toString().toStdString());
            break;
        case 5:
            _positions[index.row()].setSimulationType(value.toString().toStdString());
            break;
        }
        if(index.column()<6)
        {
            emit dataChanged(index, index);
        }
    }
    return false;
}

Qt::ItemFlags PositionTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool PositionTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);
    _positions.insert(_positions.begin()+position,rows,Position());
    endInsertRows();
    return true;
}

bool PositionTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);
    _positions.erase(_positions.begin()+position,_positions.begin()+rows+position);
    endRemoveRows();
    return true;
}

const Position &PositionTableModel::position(int i) const
{
    return _positions.at(i);
}
const std::vector<Position>& PositionTableModel::positions() const
{
    return _positions;
}


bool PositionTableModel::load(const QJsonObject &positionsListObject)
{
    if(positionsListObject.size()==0)
    {
        return true;
    }
    beginInsertRows(QModelIndex(),_positions.size(),_positions.size()+positionsListObject.size()-1);
    for(QJsonObject::const_iterator it=positionsListObject.begin(); it!=positionsListObject.end(); it++)
    {
        QJsonObject position=it.value().toObject();
        Position pos(position,it.key().toStdString());
        _positions.push_back(pos);
    }
    endInsertRows();
    return true;
}

QJsonObject PositionTableModel::jsonArray() const
{
    return Position::jsonObjects(_positions);
}
