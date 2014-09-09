#include <BALL/KERNEL/system.h>
#include <BALL/FORMAT/molFileFactory.h>
#include <BALL/FORMAT/genericMolFile.h>
#include <BALL/KERNEL/selector.h>
#include <BALL/KERNEL/PTE.h>

#include <Eigen/Core>
#include <Eigen/Dense>


#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "combination.hpp"
#include "SystemsTableModel.h"
#include "Position.h"
#include "Distance.h"

using namespace Eigen;
//using namespace BALL;

SystemsTableModel::SystemsTableModel(const DomainTableModel* domainsModel, const PositionTableModel* positionsModel, DistanceTableModel* distancesModel, QObject *parent) :
    QAbstractTableModel(parent)
{ 
    setDomainsModel(domainsModel);
    _positionsModel=positionsModel;
    connect(_positionsModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),this,SLOT(domainsUpdated()));
    _distancesModel=distancesModel;
    connect(_distancesModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),this,SLOT(domainsUpdated()));
}

int SystemsTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return systems.size();
}

int SystemsTableModel::columnCount(const QModelIndex& /*parent*/) const
{  
    return 2+numAngles();
}

QVariant SystemsTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (index.column()==0)
        {
            return systems.at(index.row()).name().c_str();
        }
        else
        {
            return cache.at(index.column()-1).at(index.row());
        }

        return QString("Row%1, Column%2")
                .arg(index.row() + 1)
                .arg(index.column() +1);
    }
    return QVariant();
}

QVariant SystemsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                return QString("Structure name");
            case 1:
                return QString("chi^2");//("χ²");
            default:
                if(section-2<(int)numEulerAngles())
                {
                    QString domainName=_domainsModel->domain((section-2)/3+1).name+":";
                    switch ((section-2)%3)
                    {
                    case 0:
                        return domainName+QString(" phi");
                    case 1:
                        return domainName+QString(" theta");
                    case 2:
                        return domainName+QString(" psi");
                    }
                }
                else
                {
                    if( int(section-2-numEulerAngles())>(int)mutualAngleNames.size())
                    {
                        std::cerr<<"WARNING! This should never happen!";
                        return "xxx";
                    }
                    return mutualAngleNames.at(section-2-numEulerAngles());
                }
            }
        }
    }
    return QVariant();
}

bool SystemsTableModel::loadSystem(const QString &fileName)
{
    MolecularSystem tmpSys;
    if(tmpSys.load(fileName))
    {
        beginInsertRows(QModelIndex(),systems.size(),systems.size());
        systems.push_back(tmpSys);
        updateCache(systems.size()-1);
        endInsertRows();
        return true;
    }
    return false;
}

bool SystemsTableModel::exportSystem(int row, const QString &filename)
{
    systems[row].save(filename);
    return true;
}

QStringList SystemsTableModel::cylinders() const
{
    QStringList list;
    int nDomains=_domainsModel->rowCount();
    for(const MolecularSystem& sys:systems)
    {
        for(int iDomain=0; iDomain<nDomains; iDomain++)
        {
            auto& domain=_domainsModel->domain(iDomain);
            QString str="CYLINDER, ";
            Eigen::Vector3d p1local(.0, .0, sys.minZ(domain));
            Eigen::Vector3d p2local(.0, .0, sys.maxZ(domain));
            Eigen::Vector3d origin=sys.pointPosition(domain,p1local);
            Eigen::Vector3d p001=sys.pointPosition(domain,p2local);
            str+=QString("%1, %2, %3, ").arg(origin[0]).arg(origin[1]).arg(origin[2]);
            str+=QString("%1, %2, %3, ").arg(p001[0]).arg(p001[1]).arg(p001[2]);
            str+="0.5, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,\n";
            list.append(str);
        }
    }
    return list;
}

QByteArray SystemsTableModel::tabSeparatedData() const
{
    QByteArray bytearr;
    int numRows=rowCount();
    int numColumns=columnCount();
    bytearr.append("Index\t");
    for(int c=0; c<numColumns; c++)
    {
        bytearr.append(headerData(c,Qt::Horizontal,Qt::DisplayRole).toString()+'\t');
    }
    bytearr.append('\n');
    for(int r=0; r<numRows; r++)
    {
        bytearr.append(headerData(r,Qt::Vertical,Qt::DisplayRole).toString()+'\t');
        for(int c=0; c<numColumns; c++)
        {
            bytearr.append(data(index(r,c)).toString()+'\t');
        }
        bytearr.append('\n');
    }
    return bytearr;
}

void SystemsTableModel::setDomainsModel(const DomainTableModel *domainsModel)
{
    _domainsModel=domainsModel;
    connect(_domainsModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),this,SLOT(domainsUpdated()));
}

void SystemsTableModel::domainsUpdated()
{
    beginResetModel();
    updateMutualAngleNames();
    updateCache(0);
    endResetModel();
}

void SystemsTableModel::updateCacheRow(int row)
{
    double tmp=chi2(row);
    cache[0][row]=tmp;
    int nDomains=_domainsModel->rowCount();
    int indexMutualAngle=numEulerAngles()+nDomains;
    for(int iDomain=1; iDomain<nDomains; iDomain++)
    {
        Eigen::Vector3d angles=systems.at(row).eulerAngles(_domainsModel->domain(iDomain),_domainsModel->domain(0));
        for(int iAngle=0; iAngle<3; iAngle++)
        {
            cache[1+3*(iDomain-1)+iAngle][row]=angles(iAngle);
        }
        cache[numEulerAngles()+iDomain][row]=angles(1);//mutual angle

        for(int iDomain2=iDomain+1; iDomain2<nDomains; iDomain2++)
        {
            angles=systems.at(row).eulerAngles(_domainsModel->domain(iDomain2),_domainsModel->domain(iDomain));
            cache[indexMutualAngle++][row]=angles(1);//mutual angle
        }
    }
}

void SystemsTableModel::updateCache(int startRow)
{
    int numCacheColumns=1+numAngles();
    cache.resize(numCacheColumns);//cache columns
    int numCacheRows=rowCount();

    for(int col=0; col<numCacheColumns; col++)
    {
        cache[col].resize(numCacheRows,std::numeric_limits<double>::quiet_NaN());
    }

    for(int row=startRow; row<rowCount(); row++)
    {
        updateCacheRow(row);
        systems[row].unload();
    }
}

void SystemsTableModel::updateMutualAngleNames()
{
    mutualAngleNames.clear();
    if(_domainsModel->rowCount()<2){
        return;
    }
    QVector<QString> domainNames=_domainsModel->names();
    QVector<QString> comb(2);
    comb[0]=domainNames.at(0);
    comb[1]=domainNames.at(1);
    do
    {
      mutualAngleNames<<(comb.at(0)+","+comb.at(1));
    }
    while(next_combination(domainNames.begin(),domainNames.end(),comb.begin(),comb.end()));
}

unsigned SystemsTableModel::numMutualAngles() const
{
    return nComb(_domainsModel->rowCount(),2);
}

unsigned SystemsTableModel::numEulerAngles() const
{
    int numEulerAngles=3*(_domainsModel->rowCount()-1);
    return numEulerAngles<0?0:numEulerAngles;
}

unsigned SystemsTableModel::numAngles() const
{
    return numEulerAngles()+numMutualAngles();
}

double SystemsTableModel::chi2(int row) const
{
    return systems.at(row).chi2(_positionsModel->positions(), _distancesModel->distances());
}

