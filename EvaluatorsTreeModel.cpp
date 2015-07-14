#include "EvaluatorsTreeModel.h"
#include "TaskStorage.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"
#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"

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
			} else if(index.column()==1) {//space for buttons
				ButtonFlags btnFlags;
				if(item.classRow()==0) {
					btnFlags.save=true;
					btnFlags.remove=true;
					btnFlags.duplicate=true;
				} else {
					btnFlags.remove=true;
					btnFlags.duplicate=true;
				}
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
					return evalRow(eval);
				}
				if(role==Qt::EditRole) {
					return evalListByType(eval);
				}
			}
			if(res.userType()==evalListType) {
				const auto& list=res.value<QList<EvalPtr>>();
				if(role==Qt::DisplayRole) {
					return QVariant::fromValue(evalRowList(list));
				}
				if(role==Qt::EditRole) {
					return evalListByType(list.front());
				}
			}
			if(res.userType()==simulationType) {
				if(role==Qt::DisplayRole) {
					const auto& type=res.value<Position::SimulationType>();
					auto name=Position::simulationTypeName(type);
					res=_simTypes.indexOf(QString::fromStdString(name));
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
		if(index.column()==1) {
			ButtonFlags btnFlags=value.value<ButtonFlags>();
			if(btnFlags.save) {
				activateEvaluator(index);
			}
			if(btnFlags.remove) {
				removeEvaluator(index);
			}
			if(btnFlags.duplicate) {
				duplicateEvaluator(index);
			}
			return true;
		}
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
			if(cr<=0) {
				data=oldData;
			} else if(value.type()==QVariant::Int) {
				newEv=eval(cr,value.toInt());
				data.setValue(newEv);
			} else if(value.type()==QVariant::String) {
				newEv=findEval(cr,value.toString());
				data.setValue(newEv);
			}
		} else if(oldData.userType()==simulationType) {
			std::string simName;
			if(value.type()==QVariant::Int) {
				simName=_simTypes[value.toInt()].toStdString();
			} else if(value.type()==QVariant::String) {
				simName=value.toString().toStdString();
			}
			const auto& simType=Position::simulationType(simName);
			data.setValue(simType);
		} else if(oldData.userType()==evalListType) {
			QList<EvalPtr> ptrs;
			int cr=classRow(oldData.value<QList<EvalPtr>>().front());
			if(value.userType()==QVariant::fromValue(QList<int>()).userType()) {
				const QList<int>& list=value.value<QList<int>>();
				for(int i:list) {
					ptrs.append(eval(cr,i));
				}
			}

			data.setValue(ptrs);
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
	if(index.column()==1 || item.isEvaluatorsClass()) {
		if(item.classRow()==0){
			return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
		} else if(item.isEvaluatorsClass() && index.column()==1){
			return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
		}
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

int EvaluatorsTreeModel::columnCount(const QModelIndex &/*parent*/) const
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
		return std::make_shared<EvaluatorDistance>(_storage,"new dist");
	case 2:
		return std::make_shared<EvaluatorChi2>(_storage,"new χ²");
	case 3:
		return std::make_shared<EvaluatorTrasformationMatrix>(_storage,"new coordinate system");
	case 4:
		return std::make_shared<EvaluatorEulerAngle>(_storage,"new euler angles");
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
	//TODO: this is all funny because TaskStorage is supposed to return only EvalIds outside, but so far...
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass()) {
		if(item.classRow()==0) {
			removeEvaluator(index.row());
			return;
		} else {
			EvalPtr ev=eval(item.classRow(),index.row());
			int evalNum=-1;
			for(int i=0; i<_storage.evalCount(); ++i) {
				if (&_storage.eval(i)==ev.get()) {
					evalNum=i;
					break;
				}
			}
			const QModelIndex& parent = classRowIndex(ev);
			beginRemoveRows(parent,index.row(),index.row());
			auto& evVec=evals[item.classRow()-1].second;
			evVec.erase(evVec.begin()+index.row());
			endRemoveRows();
			_storage.removeEvaluator(evalNum);
		}
	}
}

void EvaluatorsTreeModel::removeEvaluator(int evRow) {

	auto parent=index(0,0,QModelIndex());
	beginRemoveRows(parent,evRow,evRow);
	pendingEvals.erase(pendingEvals.begin()+evRow);
	endRemoveRows();
	return;
}


void EvaluatorsTreeModel::activateEvaluator(const QModelIndex &index)
{
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass() && item.classRow()==0) {
		_storage.addEvaluator(pendingEvals[index.row()]);
		removeEvaluator(index.row());
		loadEvaluator(_storage.evalCount()-1);
	}
}

void EvaluatorsTreeModel::activateEvaluator(int evRow)
{
	_storage.addEvaluator(pendingEvals[evRow]);
	removeEvaluator(evRow);
	loadEvaluator(_storage.evalCount()-1);
}

void EvaluatorsTreeModel::duplicateEvaluator(const QModelIndex &index)
{
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass()) {
		EvalPtr origEval=eval(item.classRow(),index.row());
		QVariantMap properties=propMap(*origEval);
		QStringList types=supportedTypes();
		int origEvalType=types.indexOf(QString::fromStdString(origEval->className()));
		addEvaluator(origEvalType);
		std::shared_ptr<AbstractEvaluator>& ev=pendingEvals.back();
		ev->setName(origEval->name()+" copy");
		setEval(pendingEvals.size()-1,properties);
	}
}

void EvaluatorsTreeModel::loadEvaluators(const QVariantMap& settings)
{
	static const QStringList classOrder{supportedTypes()};

	for(int curEvalType=0; curEvalType<classOrder.size(); curEvalType++) {
		const QString& className=classOrder.at(curEvalType);
		const QVariantMap& evals=settings[className].toMap();
		for(auto i=evals.constBegin();i!=evals.constEnd(); ++i) {
			addEvaluator(curEvalType);
			const QString& evName=i.key();
			std::shared_ptr<AbstractEvaluator>& ev=pendingEvals.back();
			ev->setName(evName.toStdString());
			const QVariantMap& propMap=i.value().toMap();
			setEval(pendingEvals.size()-1,propMap);
			if(!(propMap["isDraft"]==true)) {
				activateEvaluator(pendingEvals.size()-1);
			}
		}
	}






	/*const QModelIndex& prepClassIndex=index(0,0,QModelIndex());

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
				if(propMap.contains(propName)) {
					setData(valIndex,propMap[propName],Qt::EditRole);
				}
			}
			if(!(propMap["isDraft"]==true)) {
				activateEvaluator(evIndex);
			}
		}
	}*/
}

QVariantMap EvaluatorsTreeModel::evaluators() const
{
	QMap<QString,QVariantMap> classMap;
	const int storEvalCount=_storage.evalCount();
	for(int i=0; i<storEvalCount; ++i) {
		const auto& eval=_storage.eval(i);
		const QString& className=QString::fromStdString(eval.className());
		const QString& evName=QString::fromStdString(eval.name());
		classMap[className][evName]=propMap(eval);
	}
	for(unsigned i=0; i<pendingEvals.size(); ++i) {
		const EvalPtr& eval=pendingEvals[i];
		const QString& evName=QString::fromStdString(eval->name());
		const QString& className=QString::fromStdString(eval->className());
		QVariantMap props=propMap(*eval);
		props["isDraft"]=true;
		classMap[className][evName]=propMap(*eval);
	}
	QVariantMap settings;
	QMap<QString,QVariantMap>::const_iterator i = classMap.constBegin();
	for(;i!=classMap.constEnd();++i) {
		settings[i.key()]=i.value();
	}
	return settings;

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

EvaluatorsTreeModel::EvalPtr EvaluatorsTreeModel::eval(int classRow, int evalRow) const {
	if(classRow>0) {
		return evals[classRow-1].second[evalRow];
	} else {
		return pendingEvals[evalRow];
	}
}

EvaluatorsTreeModel::EvalPtr EvaluatorsTreeModel::findEval(int classRow, const QString& evName) const {
	if(classRow<0) {
		return EvalPtr();
	}
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

int EvaluatorsTreeModel::evalRow(const EvaluatorsTreeModel::EvalPtr &eval) const
{
	int cr=classRow(eval);
	if(cr<=0) {
		return -1;
	}
	for(size_t i=0; i<evals[cr-1].second.size(); ++i ) {
		if(evals[cr-1].second[i]==eval) {
			return i;
		}
	}
	return -1;

}

QStringList EvaluatorsTreeModel::evalListByType(const EvaluatorsTreeModel::EvalPtr &eval) const {
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

QList<int> EvaluatorsTreeModel::
evalRowList(const QList<EvaluatorsTreeModel::EvalPtr> &selected) const
{
	QList<int> list;
	int cr=classRow(selected.front());
	if(cr<=0) {
		return list;
	}
	for(size_t i=0; i<evals[cr-1].second.size(); ++i ) {
		if(selected.contains(evals[cr-1].second[i])) {
			list<<i;
		}
	}
	return list;
}

QVariantMap EvaluatorsTreeModel::propMap(const AbstractEvaluator &eval) const
{
	QVariantMap propMap;
	for(int p=0; p<eval.settingsCount(); ++p) {
		AbstractEvaluator::Setting opt=eval.setting(p);
		const QString& optName=opt.first;
		QVariant val;
		if(opt.second.userType()==evalType) {
			val=QString::fromStdString(opt.second.value<EvalPtr>()->name());

		} else if(opt.second.userType()==simulationType) {
			const auto& type=opt.second.value<Position::SimulationType>();
			val=QString::fromStdString(Position::simulationTypeName(type));
		} else if(opt.second.userType()==evalListType) {
			const auto& list=opt.second.value<QList<EvalPtr>>();
			QStringList evnames;
			for(const EvalPtr& ev:list) {
				evnames<<QString::fromStdString(ev->name());
			}
			val=evnames;
		} else if(opt.second.userType()==vec3dType) {
			const QVector3D& vec=opt.second.value<QVector3D>();
			QVariantList list;
			for(int i:{0,1,2}) {
				list.push_back(vec[i]);
			}
			val=list;
		} else {
			val=opt.second;
		}
		propMap[optName]=val;
	}
	return propMap;
}

void EvaluatorsTreeModel::setEval(int evNum, const QVariantMap &propMap) {
	std::shared_ptr<AbstractEvaluator>& ev=pendingEvals[evNum];
	for(int propRow=0; propRow<ev->settingsCount(); ++propRow) {
		QVariant oldVal=ev->settingValue(propRow);
		const QString& propName=ev->settingName(propRow);
		if(!propMap.contains(propName)) {
			continue;
		}
		const QVariant& propVal=propMap[propName];
		QVariant newVal;
		if(oldVal.userType()==evalType) {
			int cr=classRow(oldVal.value<EvalPtr>());
			newVal.setValue(findEval(cr,propVal.toString()));
		} else if(oldVal.userType()==simulationType) {
			auto simName=propVal.toString().toStdString();
			const auto& simType=Position::simulationType(simName);
			newVal.setValue(simType);
		} else if(oldVal.userType()==evalListType) {
			QList<EvalPtr> ptrs;
			int cr=classRow(oldVal.value<QList<EvalPtr>>().front());
			QStringList names=propVal.toStringList();
			for(const QString& name:names) {
				ptrs.append(findEval(cr,name));
			}
			newVal.setValue(ptrs);
		} else if(oldVal.userType()==vec3dType) {
			QVector3D vec;
			const QVariantList& list=propVal.toList();
			for(int i:{0,1,2}) {
				vec[i]=list[i].toDouble();
			}
			newVal.setValue(vec);
		} else {
			newVal=propVal;
		}
		ev->setSetting(propRow,newVal);
	}
}

