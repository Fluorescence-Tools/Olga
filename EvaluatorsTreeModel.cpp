#include "EvaluatorsTreeModel.h"
#include "TaskStorage.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"

#include <QDebug>

EvaluatorsTreeModel::EvaluatorsTreeModel(TaskStorage &storage, QObject *parent):
	QAbstractItemModel(parent),_storage(storage)
{
	for(int i=0; i<_storage.evalCount(); ++i) {
		loadEvaluator(i);
	}
}

QVariant EvaluatorsTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
		//index is class
		if(item.isInvalid()) {
			if(index.column()==0) {
				return QString::fromStdString(className(index.row()));
			}
			return QVariant();
		}

		if(item.isEvaluatorsClass()) {//index is evaluator
			if(index.column()==0) {
				return QString::fromStdString(evalName(item.classRow(),index.row()));
			} else if(index.column()==1 && item.classRow()==0) {//space for buttons
				ButtonFlags btnFlags;
				btnFlags.save=1;
				return QVariant::fromValue<ButtonFlags>(btnFlags);
			}
			return QVariant();

		}
		//index is property
		if(index.column()==0) {
			return eval(item).settingName(index.row());
		} else if (index.column()==1) {
			QVariant res=eval(item).settingValue(index.row());
			if(res.userType()==evalType) {
				const auto& eval=res.value<EvalPtr>();
				if(role==Qt::DisplayRole) {
					return QString::fromStdString(eval->name());
				}
				if(role==Qt::EditRole) {
					QStringList list;
					const auto& root=classRowIndex(eval);
					if(!root.isValid()) {
						return list;
					}
					for(int r=0; r<rowCount(root); r++){
						list<<this->index(r,0,root).data().toString();
					}
					return list;
				}
			}
			if(res.userType()==simulationType) {
				if(role==Qt::DisplayRole) {
					const auto& type=res.value<Position::SimulationType>();
					auto name=Position::simulationTypeName(type);
					res=QString::fromStdString(name);
				}
				if(role==Qt::EditRole) {
					return _simTypes;
				}
			}
			return res;
		}
	}
	return QVariant();
}
bool EvaluatorsTreeModel::setData(const QModelIndex &index, const QVariant &value,
				  int role)
{
	if(role!= Qt::EditRole) {
		return false;
	}
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass()) {
		const auto& name=value.toString().toStdString();
		pendingEvals[index.row()]->setName(name);
		return true;
	}
	Q_EMIT layoutAboutToBeChanged({parent(index)});
	if(index.data(Qt::EditRole).type()==QVariant::StringList) {
		QVariant data;
		QVariant oldData=eval(item).settingValue(index.row());
		if(oldData.userType()==evalType) {
			auto ev=oldData.value<EvalPtr>();
			int cr=classRow(ev);
			EvalPtr newEv;
			if(cr>0 && (newEv=eval(cr,value.toString()))) {
				data.setValue(newEv);
			} else {
				data=oldData;
			}
		} else if(oldData.userType()==simulationType) {
			const auto& simName=value.toString().toStdString();
			const auto& simType=Position::simulationType(simName);
			data.setValue(simType);
		}
		pendingEvals[item.evaluatorRow()]->setSetting(index.row(),data);
	} else {
		pendingEvals[item.evaluatorRow()]->setSetting(index.row(),value);
	}
	Q_EMIT layoutChanged();
	return true;
}

Qt::ItemFlags EvaluatorsTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.classRow()==0) {
		return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
	}
	return QAbstractItemModel::flags(index);
}

QVariant EvaluatorsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section)
		{
		case 0:
			return "name";
		case 1:
			return "value";
		case 2:
			return "type";
		}
	}
	return QVariant();
}

QModelIndex
EvaluatorsTreeModel::index(int row, int column, const QModelIndex &parentIndex) const
{
	if (!hasIndex(row, column, parentIndex)) {
		return QModelIndex();
	}
	//Item holds an info about the PARENT of the index, not the index itslef

	//return class
	if (!parentIndex.isValid()) {
		return createIndex(row,column,EvaluatorsTreeItem::invalidItem().intptr);
	}

	auto parentItem=EvaluatorsTreeItem::fromIntptr(parentIndex.internalId());
	//return evaluator
	if(parentItem.isInvalid()) {//the parent is a class
		auto item=EvaluatorsTreeItem::evaluatorsClass(parentIndex.row());
		return createIndex(row,column,item.intptr);
	}
	//return property
	if(parentItem.isEvaluatorsClass()) {
		EvaluatorsTreeItem item(parentItem.classRow(),parentIndex.row());
		return createIndex(row,column,item.intptr);
	}
	return QModelIndex();
}

QModelIndex EvaluatorsTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());

	if(item.isInvalid()) {//index is class
		return QModelIndex();
	}

	if(item.isEvaluatorsClass()) {//index is evaluator
		auto parentItem=EvaluatorsTreeItem::invalidItem();
		return createIndex(item.classRow(),0,parentItem.intptr);
	}
	//index is property
	auto parentItem=EvaluatorsTreeItem::evaluatorsClass(item.classRow());
	return createIndex(item.evaluatorRow(),0,parentItem.intptr);
}

int EvaluatorsTreeModel::rowCount(const QModelIndex &parent) const
{
	//number of classes
	if (!parent.isValid()) {
		return evals.size()+1;
	}
	auto parentItem=EvaluatorsTreeItem::fromIntptr(parent.internalId());
	//number of evaluators in the class
	if(parentItem.isInvalid()) {
		if(parent.row()==0) {
			return pendingEvals.size();
		}
		return evals[parent.row()-1].second.size();
	}
	//number of  settings in the evaluator
	if(parentItem.isEvaluatorsClass()) {
		if(parentItem.classRow()==0) {
			return pendingEvals[parent.row()]->settingsCount();;
		}
		return evals[parentItem.classRow()-1].second[parent.row()]->settingsCount();
	}

	//number of rows in the property
	return 0;

}

int EvaluatorsTreeModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

EvaluatorsTreeModel::~EvaluatorsTreeModel()
{

}

std::string EvaluatorsTreeModel::evalTypeName(int typeNum) const
{
	const MutableEvalPtr& eval=makeEvaluator(typeNum);
	if(eval) {
		return eval->className();
	}
	return "";
}

QStringList EvaluatorsTreeModel::supportedTypes() const {
	QStringList vec;
	int i;
	std::string typeName;
	for (i=0, typeName=evalTypeName(i);
	     !typeName.empty();
	     ++i,typeName=evalTypeName(i)){
		vec<<QString::fromStdString(typeName);
	}
	return vec;
}

void EvaluatorsTreeModel::addEvaluator(int typeNum)
{
	MutableEvalPtr eval=makeEvaluator(typeNum);
	if(eval) {
		addEvaluator(eval);
	}
}
/*
    int EvaluatorsTreeModel::classRow(const QModelIndex &index)
    {
	    auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	    return item.classRow();
}*/

QModelIndex EvaluatorsTreeModel::classRowIndex(const EvalPtr &eval) const
{
	return index(classRow(eval),0,QModelIndex());
}
/*
EvaluatorsTreeModel::EvalPtr EvaluatorsTreeModel::eval(const EvaluatorsTreeModel::EvalPtr &oldEval, int newEvalRow)
{
	int cr=classRow(oldEval)-1;
	if(cr>=0 && cr<evals.size()
	   && newEvalRow<evals[cr].second.size()) {
		return evals[cr].second[newEvalRow];
	} else {
		return EvalPtr();
	}
}*/

EvaluatorsTreeModel::MutableEvalPtr EvaluatorsTreeModel::makeEvaluator(int typeNum) const {
	switch (typeNum)
	{
	case 0:
		return std::make_shared<EvaluatorPositionSimulation>(_storage,"new LP");
	case 1:
		return std::make_shared<EvaluatorDistance>(_storage);
	case 2:
		return std::make_shared<EvaluatorChi2>(_storage,"new χ²");
	default:
		return MutableEvalPtr();
	}
}

void EvaluatorsTreeModel::addEvaluator(const std::shared_ptr<AbstractEvaluator> &eval) {
	int pendingCount=pendingEvals.size();
	beginInsertRows(index(0,0,QModelIndex()),pendingCount,pendingCount);
	pendingEvals.push_back(eval);
	endInsertRows();
}

void EvaluatorsTreeModel::removeEvaluator(const QModelIndex &index) {

	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass() && item.classRow()==0) {
		beginRemoveRows(index.parent(),index.row(),index.row());
		pendingEvals.erase(pendingEvals.begin()+index.row());
		endRemoveRows();
		return;
	} else {
		//TODO: remove evaluator from _storage
	}
}

void EvaluatorsTreeModel::saveEvaluator(const QModelIndex &index)
{
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass() && item.classRow()==0) {
		_storage.addEvaluator(pendingEvals[index.row()]);
		removeEvaluator(index);
		loadEvaluator(_storage.evalCount()-1);
	}
}

void EvaluatorsTreeModel::loadEvaluators(const QVariantMap& settings)
{
	static const QStringList classOrder{supportedTypes()};
	const QModelIndex& prepClassIndex=index(0,0,QModelIndex());

	for(int evalType=0; evalType<classOrder.size(); evalType++) {
		const QString& className=classOrder.at(evalType);
		QVariantMap evals=settings[className].toMap();
		for(auto i=evals.begin();i!= evals.constEnd(); ++i) {
			addEvaluator(evalType);
			const QString& evName=i.key();
			QModelIndex evIndex=index(rowCount(prepClassIndex)-1,0,prepClassIndex);
			setData(evIndex,evName,Qt::EditRole);
			const QVariantMap& propMap=i.value().toMap();
			for(int propRow=0; propRow<rowCount(evIndex); ++propRow) {
				const QString& propName=index(propRow,0,evIndex).data().toString();
				const QModelIndex& valIndex=index(propRow,1,evIndex);
				setData(valIndex,propMap[propName],Qt::EditRole);
			}
			if(!(propMap["isDraft"]==true)) {
				saveEvaluator(evIndex);
			}
		}
	}
}

QVariantMap EvaluatorsTreeModel::evaluators() const
{
	QMap<QString,QVariantMap> draftsClassMap;
	const QModelIndex& draftsIndex=index(0,0);
	for(int evRow=0; evRow<rowCount(draftsIndex);++evRow) {
		const QModelIndex& evIndex=index(evRow,0,draftsIndex);
		QVariantMap propMap;
		for(int propRow=0; propRow<rowCount(evIndex); ++propRow) {
			const QVariant& name=index(propRow,0,evIndex).data();
			const QVariant& val=index(propRow,1,evIndex).data();
			propMap.insert(name.toString(),val);
		}
		propMap.insert("isDraft",true);
		QString evName=evIndex.data().toString();
		const auto& className=pendingEvals[evRow]->className();
		QVariantMap &evMap=draftsClassMap[QString::fromStdString(className)];
		evName=uniqueEvalName(evMap,evName);
		evMap[evName]=propMap;
	}

	QVariantMap classMap;
	for(int classRow=1; classRow<rowCount(); ++classRow) {
		const QModelIndex& classIndex=index(classRow,0);
		const auto& className=classIndex.data().toString();
		QVariantMap evMap;
		for(int evRow=0; evRow<rowCount(classIndex); ++evRow) {
			const QModelIndex& evIndex=index(evRow,0,classIndex);
			QVariantMap propMap;
			for(int propRow=0; propRow<rowCount(evIndex); ++propRow) {
				const QVariant& name=index(propRow,0,evIndex).data();
				const QVariant& val=index(propRow,1,evIndex).data();
				propMap.insert(name.toString(),val);
			}
			QString evName=evIndex.data().toString();
			evName=uniqueEvalName(evMap,evName);
			evMap.insert(evName,propMap);
			auto i = draftsClassMap[className].constBegin();
			auto end=draftsClassMap[className].end();
			for(;i!=end;++i){
				QString evName=uniqueEvalName(evMap,i.key());
				evMap.insert(evName,i.value());
			}
		}
		classMap.insert(className,evMap);
	}

	return classMap;
}

QString EvaluatorsTreeModel::uniqueEvalName(const QVariantMap &evalMap, const QString &name) const
{
	if(evalMap.contains(name)) {
		int i=1;
		QString suf=QString::number(i);
		while(evalMap.contains(name+suf)) {
			suf=QString::number(++i);
		}
		return name+suf;
	}
	return name;
}

void EvaluatorsTreeModel::loadEvaluator(int i)
{
	auto eval=_storage.evalPtr(i);
	std::type_index classId=std::type_index(typeid(*eval));
	auto it=classRows.find(classId);
	size_t classRow;
	if(it!=classRows.end()) {
		classRow=it->second;
		const QModelIndex& parent = classRowIndex(eval);
		int row=rowCount(parent);
		beginInsertRows(parent,row,row);
		evals[classRow-1].second.push_back(eval);
		endInsertRows();
	} else {
		classRow=lastClassRow++;
		classRows.emplace(classId,classRow);
		auto vec=std::vector<EvalPtr>();
		vec.push_back(eval);
		beginInsertRows(QModelIndex(),classRow,classRow);
		evals.emplace_back(eval->className(),vec);
		endInsertRows();
	}
}

std::string EvaluatorsTreeModel::className(int classRow) const {
	if(classRow==0) {
		return "Inactive/Drafts";
	}
	return evals[classRow-1].first;
}

std::string EvaluatorsTreeModel::evalName(int classRow, int evalRow) const {
	if(classRow==0) {
		return pendingEvals[evalRow]->name();
	}
	return evals[classRow-1].second[evalRow]->name();
}

EvaluatorsTreeModel::EvalPtr EvaluatorsTreeModel::eval(int classIndex, int evalRow) const {
	return evals[classIndex-1].second[evalRow];
}
EvaluatorsTreeModel::EvalPtr EvaluatorsTreeModel::eval(int classRow, const QString& evName) const {
	std::string name=evName.toStdString();
	for(const EvalPtr& ev:evals[classRow-1].second) {
		if(ev->name()==name) {
			return ev;
		}
	}
	return EvalPtr();
}

const AbstractEvaluator &EvaluatorsTreeModel::eval(const EvaluatorsTreeItem &item) const {
	if(item.classRow()==0) {
		return *pendingEvals[item.evaluatorRow()];
	}
	return *(evals[item.classRow()-1].second[item.evaluatorRow()]);
}

int EvaluatorsTreeModel::addClassRow(const EvaluatorsTreeModel::EvalPtr &eval)
{
	std::type_index classId=std::type_index(typeid(*eval));
	auto it=classRows.find(classId);
	if(it!=classRows.end()) {
		size_t classRow=it->second;
		return classRow;
	} else {
		size_t classRow=lastClassRow++;
		classRows.emplace(classId,classRow);
		auto vec=std::vector<EvalPtr>();
		beginInsertRows(QModelIndex(),classRow,classRow);
		evals.emplace_back(eval->className(),vec);
		endInsertRows();
		return classRow;
	}
}

int EvaluatorsTreeModel::classRow(const EvaluatorsTreeModel::EvalPtr &eval) const
{
	std::type_index classId=std::type_index(typeid(*eval));
	auto it=classRows.find(classId);
	if(it!=classRows.end()) {
		size_t classRow=it->second;
		return classRow;
	} else {
		return -1;
	}
}

