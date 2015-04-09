#include "DomainTableModel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <iostream>

DomainTableModel::DomainTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

bool DomainTableModel::load(const QJsonArray &domainsArr)
{
    if(domainsArr.size()==0)
    {
	return true;
    }
    QVector<QString> _names=names();
    foreach(const QJsonValue &val,domainsArr)
    {
	QJsonObject domain=val.toObject();
	MolecularSystemDomain d(domain);
	QString name=d.name;
	if(_names.contains(name))
	{
	    std::cerr<<"Warning: domain \""<<name.toLocal8Bit().data()<<"\" is already loaded, skipping"<<std::endl;
	    continue;
	}
	//TODO: inefficient, optimize!
	beginInsertRows(QModelIndex(),_domains.size(),_domains.size());
	_names.push_back(name);
	_domains.push_back(std::make_shared<MolecularSystemDomain>(std::move(d)));
	endInsertRows();
    }

    return true;
}

QJsonArray DomainTableModel::jsonArray() const
{
    QJsonArray domains;
    for(const auto& domain:_domains)
    {
	domains.append(domain->jsonObj());
    }
    return domains;
}

const std::shared_ptr<MolecularSystemDomain> DomainTableModel::domain(int i) const
{
    return _domains.at(i);
}

QVector<QString> DomainTableModel::names() const
{
    QVector<QString> names;
    for(const auto& domain: _domains){
	names.append(domain->name);
    }
    return names;
}

int DomainTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _domains.size();
}

int DomainTableModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2;
}

QVariant DomainTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
	if (index.column()==0)
	{
	    return _domains.at(index.row())->name;
	}
	else if(index.column()==1)
	{
	    return (unsigned)_domains.at(index.row())->numPoints();
	}

	return QString("Row%1, Column%2")
		.arg(index.row() + 1)
		.arg(index.column() +1);
    }
    return QVariant();
}

QVariant DomainTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
		return QString("Domain name");
	    case 1:
		return QString("#points");
	    }
	}
    }
    return QVariant();
}

bool DomainTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
	switch(index.column())
	{
	case 0:
	    _domains[index.row()]->name=value.toString();
	    break;
	}
	if(index.column()<1)
	{
	    emit dataChanged(index, index);
	}
    }
    return false;
}

Qt::ItemFlags DomainTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	return Qt::ItemIsEnabled;

    if(index.column()<1)
    {
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    }

    return QAbstractItemModel::flags(index);
}

bool DomainTableModel::insertRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);
    _domains.insert(_domains.begin()+position,rows,
		    std::make_shared<MolecularSystemDomain>());
    endInsertRows();
    return true;
}

bool DomainTableModel::removeRows(int position, int rows, const QModelIndex &/*index*/)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);
    _domains.erase(_domains.begin()+position,_domains.begin()+rows+position);
    endRemoveRows();
    return true;
}
