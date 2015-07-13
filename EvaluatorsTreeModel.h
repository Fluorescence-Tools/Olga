#ifndef EVALUATORSTREEMODEL_H
#define EVALUATORSTREEMODEL_H

#include <vector>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include <QAbstractItemModel>

#include "AbstractEvaluator.h"
#include "TaskStorage.h"
#include "AV/Position.h"
#include "EvaluatorDelegate.h"

union EvaluatorsTreeItem {
	static_assert(sizeof(size_t)==8 || sizeof(size_t)==4,
		      "Only x64 and x32 platforms are supported");
	using size_tHalf = std::conditional<sizeof(size_t)==8, uint32_t, uint16_t>::type;

	size_t intptr;
	struct {
		size_tHalf classRow;
		size_tHalf evaluatorRow;
	} parent;

	//enum class Nesting : char {none, evaluatorClass, evaluator, property};
	EvaluatorsTreeItem(int evalClassRow, int evalRow)
	{
		parent.classRow=evalClassRow;
		parent.evaluatorRow=evalRow;
	}
	int classRow() const {
		return parent.classRow;
	}
	int evaluatorRow() const {
		return parent.evaluatorRow;
	}
	bool isEvaluatorsClass() const {
		return parent.classRow!=-1 && parent.evaluatorRow==-1;
	}
	bool isInvalid() const {
		return parent.classRow==-1 && parent.evaluatorRow==-1;
	}
	bool isEvaluator() const {
		return parent.classRow!=-1 && parent.evaluatorRow!=-1;
	}

	static EvaluatorsTreeItem invalidItem() {
		return EvaluatorsTreeItem(-1, -1);
	}
	static EvaluatorsTreeItem fromIntptr(size_t ptr) {
		return EvaluatorsTreeItem(ptr);
	}
	static EvaluatorsTreeItem evaluatorsClass(int evalClassRow) {
		return EvaluatorsTreeItem(evalClassRow,-1);
	}

private:
	EvaluatorsTreeItem(size_t ptr):intptr(ptr) {}
};
class EvaluatorsTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit EvaluatorsTreeModel(TaskStorage& storage, QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation,
			    int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column,
			  const QModelIndex &parentIndex = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	bool setData(const QModelIndex &index, const QVariant &value,
		     int role = Qt::EditRole) override;
	~EvaluatorsTreeModel();

	using ButtonFlags=EvaluatorDelegate::ButtonFlags;
	using EvalPtr=std::shared_ptr<const AbstractEvaluator>;
	using MutableEvalPtr=std::shared_ptr<AbstractEvaluator>;
	std::string evalTypeName(int typeNum) const;
	QStringList supportedTypes() const;
	void addEvaluator(int typeNum);
	//void removeEvaluator(const QModelIndex& index);
	void removeEvaluator(int evRow);
	void activateEvaluator(const QModelIndex& index);
	void activateEvaluator(int evRow);
	QVariantMap evaluators() const;
	void loadEvaluators(const QVariantMap &settings);
private:
	//EvalPtr eval(const EvalPtr& oldEval, int newEvalRow);
	QString uniqueEvalName(const QVariantMap& evalMap, const QString& name) const;
	void addEvaluator(const std::shared_ptr<AbstractEvaluator>& eval);
	MutableEvalPtr makeEvaluator(int typeNum) const;
	QModelIndex classRowIndex(const EvalPtr& eval) const;
	std::string className(int classRow) const;
	std::string evalName(int classRow, int evalRow) const;
	EvalPtr eval(int classIndex, int evalRow) const;
	EvalPtr findEval(int classIndex, const QString& evName) const;
	const AbstractEvaluator& eval(const EvaluatorsTreeItem& item) const;
	int addClassRow(const EvalPtr& eval);
	int classRow(const EvalPtr& eval) const;
	int evalRow(const EvalPtr& eval) const;
	QStringList evalListByType(const EvalPtr& eval) const;
	QList<int> evalRowList(const QList<EvalPtr>& list) const;
	QVariantMap propMap(const AbstractEvaluator &eval) const;


	void loadEvaluator(int i);
	TaskStorage& _storage;
	size_t lastClassRow=1;
	std::unordered_map<std::type_index,size_t> classRows;
	//evals[classIndex][evalNum]==eval;
	std::vector<
	std::pair<std::string,std::vector<EvalPtr>>
	> evals;

	std::vector<std::shared_ptr<AbstractEvaluator>> pendingEvals;

	const int evalType=QVariant::fromValue(EvalPtr()).userType();
	const int simulationType=QVariant::fromValue(Position::SimulationType()).userType();
	const int evalListType=QVariant::fromValue(QList<EvalPtr>()).userType();
	const QStringList _simTypes{
		QString::fromStdString(Position::simulationTypeName(Position::SimulationType::av1)),
		QString::fromStdString(Position::simulationTypeName(Position::SimulationType::av3)),
		QString::fromStdString(Position::simulationTypeName(Position::SimulationType::atom))
	};
};

#endif // EVALUATORSTREEMODEL_H
