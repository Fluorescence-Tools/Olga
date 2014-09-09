#ifndef SYSTEMSTABLEMODEL_H
#define SYSTEMSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>
#include <QItemSelectionModel>

#include <MolecularSystem.h>
#include <MolecularSystemDomain.h>
#include "DomainTableModel.h"
#include "DistanceTableModel.h"
#include "PositionTableModel.h"

class SystemsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SystemsTableModel(const DomainTableModel* domainsModel, const PositionTableModel* positionsModel, DistanceTableModel* distancesModel, QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool loadSystem(const QString& fileName);
    bool exportSystem(int row, const QString& filename);

    QStringList cylinders() const;
    QByteArray tabSeparatedData() const;

signals:

public slots:
    void domainsUpdated();
private:
    void setDomainsModel(const DomainTableModel* domainsModel);
    void updateCacheRow(int row);
    void updateCache(int startRow=0);
    void updateMutualAngleNames();
    unsigned numMutualAngles() const;
    unsigned numEulerAngles() const;
    unsigned numAngles() const;
    double chi2(int row) const;

private:
    QVector<MolecularSystem> systems;
    const DomainTableModel* _domainsModel;
    const PositionTableModel* _positionsModel;
    const DistanceTableModel* _distancesModel;
    mutable std::vector<std::vector<double>> cache;
    mutable QVector<QString> mutualAngleNames;
};

#endif // SYSTEMSTABLEMODEL_H
