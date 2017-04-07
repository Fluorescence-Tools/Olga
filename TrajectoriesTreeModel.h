#ifndef TRAJECTORIESTREEMODEL_H
#define TRAJECTORIESTREEMODEL_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QVector>
#include <QTimer>

#include "MolecularTrajectory.h"
#include "AbstractEvaluator.h"
#include "AbstractCalcResult.h"
#include "TaskStorage.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"
#include "TrajectoriesTreeItem.h"
#include "FrameDescriptor.h"
#include "AV/MolecularSystemDomain.h"

class TrajectoriesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit TrajectoriesTreeModel(const TaskStorage& storage, QObject *parent = 0);

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
	void dumpTabSeparatedData(QTextStream& out) const;
	int tasksRunningCount() const
	{
		return _storage.tasksRunningCount();
	}
	int tasksPendingCount() const
	{
		return _storage.tasksPendingCount();
	}
	int resultsCount() const
	{
		return _storage.resultCount();
	}
	int sysTasksCount() const
	{
		return _storage.sysTaskCount();
	}
	/*void updateColumn(int column);
	void updateRow(const QModelIndex & parent, int row);
	*/
	pteros::System firstSystem() const
	{
		pteros::System system;
		if(_molTrajs.empty()) {
			return system;
		}
		if (_molTrajs[0].empty()) {
			return system;
		}
		auto frame=frameDescriptor(childItem(nullptr,0), 0);
		try {
			return pteros::System(frame.topologyFileName());
		}
		catch (...) {
			return system;
		}

	}
	std::vector<FrameDescriptor> frames() const
	{
		std::vector<FrameDescriptor> res;
		for(const MolecularTrajectory& mt:_molTrajs)//iterate over all frames in trajectories
		{
			for(int trajIdx=0; trajIdx<mt.chunkCount(); ++trajIdx) {
				for(int frIdx=0; frIdx<mt.frameCount(trajIdx); ++frIdx) {
					auto desc=mt.descriptor(trajIdx,frIdx);
					res.push_back(std::move(desc));
				}
			}
		}
		return res;
	}
public Q_SLOTS:
private Q_SLOTS:

private:
	void evaluatorAdded(const EvalId& id);
	void evaluatorRemove(const EvalId& id);
	const TrajectoriesTreeItem* childItem(const TrajectoriesTreeItem* parent,
					      unsigned row) const;

	QString frameName(const TrajectoriesTreeItem *parent, int row) const;
	QString colName(int section) const;
	FrameDescriptor frameDescriptor(const TrajectoriesTreeItem *parent, int row) const;
	std::shared_ptr<AbstractCalcResult>
	calculate(const FrameDescriptor desc,
		  const std::shared_ptr<AbstractEvaluator> calc) const;
	//pteros::System system(const FrameDescriptor &desc) const;
private:
	QVector<MolecularTrajectory> _molTrajs;
	mutable std::unordered_set<TrajectoriesTreeItem> items;
	using CalcColumn=std::pair<EvalId,int>;//EvId,calcCol
	std::vector<CalcColumn> _columns;

	const TaskStorage& _storage;

	std::vector<EvalId> _evalsPending;
	QTimer _evaluatePending{this};

};

#endif // TRAJECTORIESTREEMODEL_H
