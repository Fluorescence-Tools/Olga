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
	static_assert(sizeof(size_t) == 8 || sizeof(size_t) == 4,
		      "Only x64 and x32 platforms are supported");
	using size_tHalf =
		std::conditional<sizeof(size_t) == 8, uint32_t, uint16_t>::type;
	static constexpr size_tHalf Half_Max()
	{
		return pow(2, sizeof(size_tHalf) * 8) - 1;
	}
	size_t intptr;
	struct {
		size_tHalf classRow;
		size_tHalf evaluatorRow;
	} parent;

	// enum class Nesting : char {none, evaluatorClass, evaluator,
	// property};
	EvaluatorsTreeItem(int evalClassRow, int evalRow)
	{
		parent.classRow = evalClassRow;
		parent.evaluatorRow = evalRow;
	}
	int classRow() const
	{
		return parent.classRow;
	}
	int evaluatorRow() const
	{
		return parent.evaluatorRow;
	}
	bool isEvaluatorsClass() const
	{
		return parent.classRow != Half_Max()
		       && parent.evaluatorRow == Half_Max();
	}
	bool isInvalid() const
	{
		return parent.classRow == Half_Max()
		       && parent.evaluatorRow == Half_Max();
	}
	bool isEvaluator() const
	{
		return parent.classRow != Half_Max()
		       && parent.evaluatorRow != Half_Max();
	}

	static EvaluatorsTreeItem invalidItem()
	{
		return EvaluatorsTreeItem(Half_Max(), Half_Max());
	}
	static EvaluatorsTreeItem fromIntptr(size_t ptr)
	{
		return EvaluatorsTreeItem(ptr);
	}
	static EvaluatorsTreeItem evaluatorsClass(int evalClassRow)
	{
		return EvaluatorsTreeItem(evalClassRow, Half_Max());
	}

private:
	EvaluatorsTreeItem(size_t ptr) : intptr(ptr)
	{
	}
};
class EvaluatorsTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit EvaluatorsTreeModel(TaskStorage &storage, QObject *parent = 0);
	QVariant data(const QModelIndex &index,
		      int role = Qt::DisplayRole) const;
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

	using ButtonFlags = EvaluatorDelegate::ButtonFlags;
	using MutableEvalPtr = std::unique_ptr<AbstractEvaluator>;

	EvalId evalId(const QModelIndex &index) const;

	void addEvaluator(int typeNum);
	template <typename T> QModelIndex addEvaluator()
	{
		MutableEvalPtr eval = _storage.makeEvaluator<T>();
		if (eval) {
			addEvaluator(std::move(eval));
			QModelIndex drafts = index(0, 0);
			return index(pendingEvals.size() - 1, 0, drafts);
		}
		return QModelIndex();
	}
	void removeEvaluator(const QModelIndex &index);
	void removeEvaluator(const EvalId &ev);
	MutableEvalPtr removeEvaluator(int evRow);
	void setEvaluatorName(const QModelIndex &index,
			      const std::string &name);
	std::string evalName(const QModelIndex &index);
	void activateEvaluator(const QModelIndex &index);
	void activateEvaluator(int evRow);
	void setEvaluatorOption(const QModelIndex &index,
				const QString &optionName,
				const QVariant &value);
	QModelIndex duplicateEvaluator(const QModelIndex &index);
	QVariantMap evaluators() const;
	void loadEvaluators(const QVariantMap &settings);
	QVariantMap evaluatorsFromLegacy(QTextStream &in) const;
	template <typename T> QList<QModelIndex> evaluatorsAvailable() const
	{
		QList<QModelIndex> list;
		// T eval(_storage,"none");
		MutableEvalPtr eval = _storage.makeEvaluator<T>();
		auto classId = std::type_index(typeid(*eval));
		auto it = classRows.find(classId);
		size_t classRow = -1;
		if (it != classRows.end()) {
			classRow = it->second;
		} else {
			return list;
		}
		const auto &root = index(classRow, 0, QModelIndex());
		if (!root.isValid()) {
			return list;
		}
		for (int r = 0; r < rowCount(root); r++) {
			list << this->index(r, 0, root);
		}
		return list;
	}
	int evaluatorsPendingCount() const
	{
		return pendingEvals.size();
	}

private:
	// EvalPtr eval(const EvalPtr& oldEval, int newEvalRow);
	QString uniqueEvalName(const QVariantMap &evalMap,
			       const QString &name) const;
	void addEvaluator(std::unique_ptr<AbstractEvaluator> eval);
	QModelIndex classRowIndex(const EvalId &eval) const;
	std::string className(int classRow) const;
	std::string evalName(int classRow, int evalRow) const;
	const AbstractEvaluator &eval(int classIndex, int evalRow) const;
	EvalId evalId(int classRow, int evalRow) const;
	EvalId findEval(int classIndex, const QString &evName) const;
	const AbstractEvaluator &eval(const EvaluatorsTreeItem &item) const;
	int addClassRow(const EvalId &evId);
	int classRow(const EvalId &evId) const;
	int evalRow(const EvalId &eval) const;
	QStringList evalListByType(const EvalId &eval) const;
	QList<int> evalRowList(const QList<EvalId> &list) const;
	// QVariantMap propMap(const AbstractEvaluator &eval) const;
	// void setEval(int evNum,const QVariantMap& propMap);
	void loadEvaluator(EvalId id);
	TaskStorage &_storage;
	size_t lastClassRow = 1;
	std::unordered_map<std::type_index, size_t> classRows;
	// evals[classIndex][evalNum]==eval;
	std::vector<std::pair<std::string, std::vector<EvalId>>> evals;

	std::vector<MutableEvalPtr> pendingEvals;

	const int evalType = QVariant::fromValue(EvalId()).userType();
	const int simulationType =
		QVariant::fromValue(Position::SimulationType()).userType();
	const int evalListType =
		QVariant::fromValue(QList<EvalId>()).userType();
	const int vec3dType = QVariant::fromValue(QVector3D()).userType();
	const QStringList _simTypes{
		QString::fromStdString(Position::simulationTypeName(
			Position::SimulationType::AV1)),
		QString::fromStdString(Position::simulationTypeName(
			Position::SimulationType::AV3)),
		QString::fromStdString(Position::simulationTypeName(
			Position::SimulationType::ATOM))};
};

#endif // EVALUATORSTREEMODEL_H
