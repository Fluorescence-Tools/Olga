#include "DistanceTableModel.h"
#include <QJsonArray>
#include <QCollator>
DistanceTableModel::DistanceTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

int DistanceTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _distances.size();
}

int DistanceTableModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 8;
}

QVariant DistanceTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
	switch(index.column())
	{
	case 0:
	    return QString::fromStdString(_distances.at(index.row())->name());
	case 1:
	    return QString::fromStdString(_distances.at(index.row())->position1());
	case 2:
	    return QString::fromStdString(_distances.at(index.row())->position2());
	case 3:
	    return _distances.at(index.row())->distance();
	case 4:
	    return _distances.at(index.row())->errNeg();
	case 5:
	    return _distances.at(index.row())->errPos();
	case 6:
	    return QString::fromStdString(_distances.at(index.row())->type());
	case 7:
	    return _distances.at(index.row())->R0();
	}

	return QString("Row%1, Column%2")
		.arg(index.row() + 1)
		.arg(index.column() +1);
    }
    return QVariant();
}

QVariant DistanceTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
		return QString("Name");
	    case 1:
		return QString("Position #1");
	    case 2:
		return QString("Position #2");
	    case 3:
		return QString("Distance");
	    case 4:
		return QString("Err-");
	    case 5:
		return QString("Err+");
	    case 6:
		return QString("Type");
	    case 7:
		return QString("R₀");
	    }
	}
    }
    return QVariant();
}

bool DistanceTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
	switch(index.column())
	{
	case 0:
	    _distances[index.row()]->setName(value.toString().toStdString());
	    break;
	case 1:
	    _distances[index.row()]->setPosition1(value.toString().toStdString());
	    break;
	case 2:
	    _distances[index.row()]->setPosition2(value.toString().toStdString());
	    break;
	case 3:
	    _distances[index.row()]->setDistance(value.toDouble());
	    break;
	case 4:
	    _distances[index.row()]->setErrNeg(value.toDouble());
	    break;
	case 5:
	    _distances[index.row()]->setErrPos(value.toDouble());
	    break;
	case 6:
	    _distances[index.row()]->setType(value.toString().toStdString());
	    break;
	case 7:
	    _distances[index.row()]->setR0(value.toDouble());
	    break;
	}
	if(index.column()<8)
	{
	    emit dataChanged(index, index);
	}
    }
    return false;
}

Qt::ItemFlags DistanceTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool DistanceTableModel::insertRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);
    _distances.insert(_distances.begin()+position,rows,std::make_shared<Distance>());
    endInsertRows();
    return true;
}

bool DistanceTableModel::removeRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);
    _distances.erase(_distances.begin()+position,_distances.begin()+rows+position);
    endRemoveRows();
    return true;
}

bool DistanceTableModel::load(const QJsonObject &distancesListObject)
{
    if(distancesListObject.size()==0)
    {
	return true;
    }
    beginInsertRows(QModelIndex(),_distances.size(),_distances.size()+distancesListObject.size()-1);
    for(QJsonObject::const_iterator it=distancesListObject.begin(); it!=distancesListObject.end(); it++)
    {
	QJsonObject distance=it.value().toObject();
	_distances.push_back(std::make_shared<Distance>(distance,it.key().toStdString()));
    }
    QCollator collator;
    collator.setNumericMode(true);
    //TODO: this is a poor man's solution, it is better to make the program
    //rememeber the ordering from the settings file, bu QJson does not support it atm.
    std::sort(_distances.begin(), _distances.end(),
	[&collator](const std::shared_ptr<Distance> & a, const std::shared_ptr<Distance> & b) -> bool
    {
	return -1==collator.compare(QString::fromStdString(a->name()),QString::fromStdString(b->name()));
	//    return a.name()<b.name();
    });
    endInsertRows();
    return true;
}

QJsonObject DistanceTableModel::jsonArray() const
{
    return Distance::jsonObjects(_distances);
}
