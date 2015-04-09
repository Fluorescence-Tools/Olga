#ifndef TRAJECTORIESTREEMODEL_H
#define TRAJECTORIESTREEMODEL_H
#include "ThreadPool.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QVector>

#include "MolecularTrajectory.h"
#include "AbstractCalculator.h"
#include "CalculatorPositionSimulation.h"
#include "TrajectoriesTreeItem.h"
#include "FrameDescriptor.h"
#include "AV/MolecularSystemDomain.h"
#include "DomainTableModel.h"
#include "DistanceTableModel.h"
#include "PositionTableModel.h"
#include "CalculatorDistance.h"
#include "CalculatorChi2.h"


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
		(void)row; (void)filename;
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
	void calculationFinished(const FrameDescriptor desc,
				 const std::shared_ptr<AbstractCalculator> calc,
				 std::shared_ptr<AbstractCalcResult> result,
				 const QModelIndex index);
	void calculationFinished(const FrameDescriptor desc,
				 const std::shared_ptr<AbstractCalculator> calc,
				 std::shared_ptr<AbstractCalcResult> result);
public slots:
private slots:
	void domainsInserted(int from, int to)
	{
		for(int i=from; i<=to; i++)
		{
			addCalculator(_domainsModel->domain(i));
		}
	}
	void positionsInserted(int from, int to)
	{
		for(int i=from; i<=to; i++)
		{
			addCalculator(_positionsModel->position(i));
		}
	}
	void distancesInserted(int from, int to)
	{
		for(int i=from; i<=to; i++)
		{
			addCalculator(_distancesModel->distance(i));
		}
	}
	bool updateCache(const FrameDescriptor desc,
			 const std::shared_ptr<AbstractCalculator> calc,
			 std::shared_ptr<AbstractCalcResult> result);
	bool updateCache(const FrameDescriptor desc,
			 const std::shared_ptr<AbstractCalculator> calc,
			 std::shared_ptr<AbstractCalcResult> result,
			 const QModelIndex index);

private:
	const TrajectoriesTreeItem* childItem(const TrajectoriesTreeItem* parent,
					      unsigned row) const;

	QString frameName(const TrajectoriesTreeItem *parent, int row) const;
	QString calculatorName(int calcNum) const;
	FrameDescriptor frameDescriptor(const TrajectoriesTreeItem *parent, int row) const;
	void addCalculator(const std::shared_ptr<MolecularSystemDomain> domain);
	void addCalculator(const std::shared_ptr<Position> position);
	void addCalculator(const std::shared_ptr<Distance> distance);
	//void recalculateColumn(int column) const;
	void recalculate(const std::shared_ptr<AbstractCalculator> calc) const;
	void recalculateRow(const QModelIndex &index) const;
	std::shared_ptr<AbstractCalcResult>
	calculate(const FrameDescriptor desc,
		  const std::shared_ptr<AbstractCalculator> calc) const;
	void appendTask(const FrameDescriptor desc,
			const std::shared_ptr<AbstractCalculator> calc,
			const QModelIndex index) const;
	void appendTask(const FrameDescriptor desc,
			const std::shared_ptr<AbstractCalculator> calc) const;
	//void appendTask(const QModelIndex index) const;
	pteros::System system(const FrameDescriptor &desc) const;
	using CalcColumn=std::pair<std::shared_ptr<AbstractCalculator>,int>;
	using ResultCacheCol=std::unordered_map<std::shared_ptr<AbstractCalculator>,
	std::shared_ptr<AbstractCalcResult>>;
	using ResultCache=std::unordered_map<FrameDescriptor,ResultCacheCol>;
	ResultCacheCol::iterator cachedEntry(const FrameDescriptor& desc, const std::shared_ptr<AbstractCalculator> calc) const;
private:
	QVector<MolecularTrajectory> _molTrajs;
	mutable std::unordered_set<TrajectoriesTreeItem> items;
	std::unordered_set<std::shared_ptr<AbstractCalculator>> _calculators;//calculator,column
	std::vector<CalcColumn> _visibleCalculators;
	mutable ResultCache cache;//[FrameDescriptor][AbstractCalculator]

	std::unordered_map<std::string,std::weak_ptr<CalculatorPositionSimulation>> _avCalculators;//lp_name

	const DomainTableModel* _domainsModel;
	const PositionTableModel* _positionsModel;
	const DistanceTableModel* _distancesModel;

	mutable ThreadPool threadPool;

	std::vector<std::weak_ptr<CalculatorDistance>> _distanceCalculators;
	std::shared_ptr<CalculatorChi2> _chi2Calc;
};

#endif // TRAJECTORIESTREEMODEL_H
