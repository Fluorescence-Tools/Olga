#ifndef TRAJECTORIESTREEMODEL_H
#define TRAJECTORIESTREEMODEL_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QVector>

#include "MolecularTrajectory.h"
#include "AbstractCalculator.h"
#include "TrajectoriesTreeItem.h"
#include "FrameDescriptor.h"
#include "AV/MolecularSystemDomain.h"
#include "DomainTableModel.h"
#include "DistanceTableModel.h"
#include "PositionTableModel.h"
#include "ThreadPool.h"

class TrajectoriesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit TrajectoriesTreeModel(const DomainTableModel* domainsModel,
				       const PositionTableModel* positionsModel,
				       DistanceTableModel* distancesModel,
				       QObject *parent = 0);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation,
			    int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column,
			  const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	bool loadSystem(const QString& fileName);
	bool exportSystem(int row, const QString& filename)
	{
		return false;//TODO: implement
	}
	QStringList cylinders() const
	{
		return QStringList();//TODO: implement
	}
	QByteArray tabSeparatedData() const
	{
		return QByteArray();//TODO: implement
	}


signals:
	void calculationFinished(const QModelIndex index, std::shared_ptr<AbstractCalcResult> result);
public slots:
private slots:
	void domainsInserted(int from, int to)
	{
		std::cerr<<"domains inserted:"<<std::to_string(from)<<"-"<<
			   std::to_string(to)<<std::endl;
		for(int i=from; i<=to; i++)
		{
			addCalculator(_domainsModel->domain(i));
		}
	}
	void updateCache(const QModelIndex index, std::shared_ptr<AbstractCalcResult> result);

private:
	const TrajectoriesTreeItem* childItem(const TrajectoriesTreeItem* parent,
					      unsigned row) const;
	QString frameName(const TrajectoriesTreeItem *parent, int row) const;
	QString calculatorName(int calcNum) const;
	FrameDescriptor frameDescriptor(const TrajectoriesTreeItem *parent, int row) const;
	void addCalculator(const std::weak_ptr<MolecularSystemDomain> domain);
	void recalculateColumn(int column) const;
	void appendTask(const QModelIndex index) const;
	pteros::System system(const FrameDescriptor &desc) const;
private:
	QVector<MolecularTrajectory> _molTrajs;
	mutable std::unordered_set<TrajectoriesTreeItem> items;
	std::unordered_set<std::shared_ptr<AbstractCalculator>> _calculators;
	std::vector<std::shared_ptr<AbstractCalculator>> _visibleCalculators;
	using ResultCacheCol=std::unordered_map<std::shared_ptr<AbstractCalculator>,
					std::shared_ptr<AbstractCalcResult>>;
	using ResultCache=std::unordered_map<FrameDescriptor,ResultCacheCol>;

	mutable ResultCache cache;//[FrameDescriptor][AbstractCalculator]

	const DomainTableModel* _domainsModel;
	const PositionTableModel* _positionsModel;
	const DistanceTableModel* _distancesModel;

	mutable ThreadPool threadPool;
};

#endif // TRAJECTORIESTREEMODEL_H
