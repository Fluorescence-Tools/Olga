#ifndef TRAJECTORIESTREEMODEL_H
#define TRAJECTORIESTREEMODEL_H
//#include "ThreadPool.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QVector>

#include "MolecularTrajectory.h"
#include "AbstractEvaluator.h"
#include "AbstractCalcResult.h"
#include "TaskStorage.h"
//#include "AbstractCalculator.h"
//#include "CalculatorPositionSimulation.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"
#include "TrajectoriesTreeItem.h"
#include "FrameDescriptor.h"
#include "AV/MolecularSystemDomain.h"
#include "DomainTableModel.h"
#include "DistanceTableModel.h"
#include "PositionTableModel.h"
//#include "CalculatorDistance.h"
//#include "CalculatorChi2.h"


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
	QByteArray tabSeparatedData() const;
	int tasksCount() const
	{
		return _storage.numTasks();
	}
	int tasksCountSubmited() const
	{
		return _storage.numTasksSubmited();
	}
	int tasksCountFinished() const
	{
		return _storage.numTasksFinished();
	}
	int sysTasksCount() const
	{
		return _storage.numSysTasks();
	}
	void updateColumn(int column);

public slots:
private slots:
	void domainsInserted(int from, int to);
	void positionsInserted(int from, int to);
	void distancesInserted(int from, int to);

private:
	const TrajectoriesTreeItem* childItem(const TrajectoriesTreeItem* parent,
					      unsigned row) const;

	QString frameName(const TrajectoriesTreeItem *parent, int row) const;
	QString calculatorName(int calcNum) const;
	FrameDescriptor frameDescriptor(const TrajectoriesTreeItem *parent, int row) const;
	void addCalculator(const std::shared_ptr<MolecularSystemDomain> domain);
	void addCalculator(const std::shared_ptr<Position> position);
	void addCalculator(const std::shared_ptr<Distance> distance);
	std::shared_ptr<AbstractCalcResult>
	calculate(const FrameDescriptor desc,
		  const std::shared_ptr<AbstractEvaluator> calc) const;
	void appendTask(const FrameDescriptor desc,
			const std::shared_ptr<AbstractEvaluator> calc,
			const QModelIndex index) const;
	void appendTask(const FrameDescriptor desc,
			const std::shared_ptr<AbstractEvaluator> calc) const;
	//void appendTask(const QModelIndex index) const;
	pteros::System system(const FrameDescriptor &desc) const;
private:
	QVector<MolecularTrajectory> _molTrajs;
	mutable std::unordered_set<TrajectoriesTreeItem> items;
	std::unordered_set<std::shared_ptr<AbstractEvaluator>> _calculators;//calculator,column
	using CalcColumn=std::pair<std::shared_ptr<AbstractEvaluator>,int>;
	std::vector<CalcColumn> _visibleCalculators;

	std::unordered_map<std::string,std::weak_ptr<EvaluatorPositionSimulation>> _avCalculators;//lp_name

	const DomainTableModel* _domainsModel;
	const PositionTableModel* _positionsModel;
	const DistanceTableModel* _distancesModel;

	mutable TaskStorage _storage;
	//mutable ThreadPool threadPool;

	std::vector<std::weak_ptr<EvaluatorDistance>> _distanceCalculators;
	std::shared_ptr<EvaluatorChi2> _chi2Calc;
};

#endif // TRAJECTORIESTREEMODEL_H
