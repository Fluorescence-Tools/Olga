#include "EvaluatorsTreeModel.h"
#include "TaskStorage.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"
#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"

#include <QFileDialog>

#include <memory>

EvaluatorsTreeModel::EvaluatorsTreeModel(TaskStorage &storage, QObject *parent):
	QAbstractItemModel(parent),_storage(storage)
{
	connect(&_storage,&TaskStorage::evaluatorAdded,[this](const EvalId& id){
		loadEvaluator(id);
	});
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
				const auto& eval=res.value<EvalId>();
				if(role==Qt::DisplayRole) {
					return evalRow(eval);
				}
				if(role==Qt::EditRole) {
					return evalListByType(eval);
				}
			}
			if(res.userType()==evalListType) {
				const auto& list=res.value<QList<EvalId>>();
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
			auto ev=oldData.value<EvalId>();
			int cr=classRow(ev);
			EvalId newEv;
			if(cr<=0) {
				data=oldData;
			} else if(value.type()==QVariant::Int) {
				int evalRow=value.toInt();
				if(evalRow>=0) {
					newEv=evalId(cr,value.toInt());
					data.setValue(newEv);
				} else {
					data=oldData;
				}
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
			QList<EvalId> ptrs;
			int cr=classRow(oldData.value<QList<EvalId>>().front());
			if(value.userType()==QVariant::fromValue(QList<int>()).userType()) {
				const QList<int>& list=value.value<QList<int>>();
				for(int i:list) {
					ptrs.append(evalId(cr,i));
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
			return pendingEvals[parent.row()]->settingsCount();
		}
		const EvalId& id=evals[parentItem.classRow()-1].second[parent.row()];
		return _storage.eval(id).settingsCount();
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


void EvaluatorsTreeModel::addEvaluator(int typeNum)
{
	MutableEvalPtr eval=_storage.makeEvaluator(typeNum);
	if(eval) {
		addEvaluator(std::move(eval));
	}
}
/*
    int EvaluatorsTreeModel::classRow(const QModelIndex &index)
    {
	    auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	    return item.classRow();
}*/

QModelIndex EvaluatorsTreeModel::classRowIndex(const EvalId &eval) const
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

void EvaluatorsTreeModel::addEvaluator(std::unique_ptr<AbstractEvaluator> eval) {
	int pendingCount=pendingEvals.size();
	beginInsertRows(index(0,0,QModelIndex()),pendingCount,pendingCount);
	pendingEvals.push_back(std::move(eval));
	endInsertRows();
}

void EvaluatorsTreeModel::removeEvaluator(const QModelIndex &index) {
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass()) {
		if(item.classRow()==0) {
			removeEvaluator(index.row());
			return;
		} else {
			EvalId ev=evalId(item.classRow(),index.row());
			const QModelIndex& parent = classRowIndex(ev);
			beginRemoveRows(parent,index.row(),index.row());
			auto& evVec=evals[item.classRow()-1].second;
			evVec.erase(evVec.begin()+index.row());
			endRemoveRows();
			_storage.removeEvaluator(ev);
		}
	}
}

EvaluatorsTreeModel::MutableEvalPtr EvaluatorsTreeModel::removeEvaluator(int evRow) {

	auto parent=index(0,0,QModelIndex());
	beginRemoveRows(parent,evRow,evRow);
	auto it=pendingEvals.begin()+evRow;
	MutableEvalPtr evptr=std::move(*it);
	pendingEvals.erase(it);
	endRemoveRows();
	return evptr;
}


void EvaluatorsTreeModel::activateEvaluator(const QModelIndex &index)
{
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass() && item.classRow()==0) {
		auto evId=_storage.addEvaluator(removeEvaluator(index.row()));
	}
}
/*
void EvaluatorsTreeModel::activateEvaluator(int evRow)
{
	loadEvaluator(_storage.addEvaluator(removeEvaluator(evRow)));
}*/

void EvaluatorsTreeModel::duplicateEvaluator(const QModelIndex &index)
{
	auto item=EvaluatorsTreeItem::fromIntptr(index.internalId());
	if(item.isEvaluatorsClass()) {
		const auto& origEval=eval(item.classRow(),index.row());
		QVariantMap properties=_storage.propMap(origEval);
		QStringList types=_storage.supportedTypes();
		int origEvalType=types.indexOf(QString::fromStdString(origEval.className()));
		addEvaluator(origEvalType);
		MutableEvalPtr& ev=pendingEvals.back();
		ev->setName(origEval.name()+" copy");
		_storage.setEval(ev,properties);
	}
}

void EvaluatorsTreeModel::loadEvaluators(const QVariantMap& settings)
{
	std::vector<MutableEvalPtr> evs=_storage.loadEvaluators(settings);
	for(auto& ev:evs) {
		addEvaluator(std::move(ev));
	}/*
	//TODO:split between here and TaskStorage
	static const QStringList classOrder{supportedTypes()};

	for(int curEvalType=0; curEvalType<classOrder.size(); curEvalType++) {
		const QString& className=classOrder.at(curEvalType);
		const QVariantMap& evals=settings[className].toMap();
		for(auto i=evals.constBegin();i!=evals.constEnd(); ++i) {
			addEvaluator(curEvalType);
			const QString& evName=i.key();
			MutableEvalPtr& ev=pendingEvals.back();
			ev->setName(evName.toStdString());
			const QVariantMap& propMap=i.value().toMap();
			setEval(pendingEvals.size()-1,propMap);
			if(!(propMap["isDraft"]==true)) {
				activateEvaluator(pendingEvals.size()-1);
			}
		}
	}*/
}

QVariantMap EvaluatorsTreeModel::evaluatorsFromLegacy(QTextStream &in) const
{
	QVariantMap evals;

	QStringList lines=in.readAll().split('\n');
	lines[0]=lines[0].trimmed();
	if (lines[0]=="Rmp" || lines[0]=="RDAMean" || lines[0]=="RDAMeanE") {
		QVariantMap propMap;
		QString type=lines.takeFirst();
		for( const QString& line:lines) {
			if(line.trimmed().isEmpty()) {
				continue;
			}
			auto eval=EvaluatorDistance(_storage,line,type);
			const QString& evName=QString::fromStdString(eval.name());
			if(!evName.isEmpty()) {
				propMap[evName]=_storage.propMap(eval);
			}
		}
		const QString& className=QString::fromStdString(EvaluatorDistance(_storage,"").className());
		evals[className]=propMap;
	}
	else {//Labelling Postions
		QString pdb=QFileDialog::getOpenFileName(0,
					     tr("Open PDB file, corresponding to the Labelling file"), "",
					     tr("Protein Data Bank (*.pdb);;All Files (*)"));
		if(pdb.isEmpty()) {
			return evals;
		}
		QVariantMap propMap;
		for( const QString& line:lines) {
			if(line.trimmed().isEmpty()) {
				continue;
			}
			auto eval=EvaluatorPositionSimulation(_storage,line,pdb);
			const QString& evName=QString::fromStdString(eval.name());
			if(!evName.isEmpty()) {
				propMap[evName]=_storage.propMap(eval);
			}
		}
		const QString& className=QString::fromStdString(EvaluatorPositionSimulation(_storage,"").className());
		evals[className]=propMap;
	}
	return evals;
}

QVariantMap EvaluatorsTreeModel::evaluators() const
{
	QMap<QString,QVariantMap> classMap;
	for(const auto& pair:_storage.evals()) {
		if(_storage.isStub(pair.first)) {
			continue;
		}
		const auto& eval=*(pair.second);
		const QString& className=QString::fromStdString(eval.className());
		const QString& evName=QString::fromStdString(eval.name());
		classMap[className][evName]=_storage.propMap(eval);
	}
	for(unsigned i=0; i<pendingEvals.size(); ++i) {
		const MutableEvalPtr& eval=pendingEvals[i];
		const QString& evName=QString::fromStdString(eval->name());
		const QString& className=QString::fromStdString(eval->className());
		QVariantMap props=_storage.propMap(*eval);
		props["isDraft"]=true;
		classMap[className][evName]=_storage.propMap(*eval);
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

void EvaluatorsTreeModel::loadEvaluator(EvalId id)
{
	//TODO:check if id is already loaded
	const auto& eval=_storage.eval(id);
	std::type_index classId=std::type_index(typeid(eval));
	auto it=classRows.find(classId);
	size_t classRow;
	if(it!=classRows.end()) {
		classRow=it->second;
		const QModelIndex& parent = classRowIndex(id);
		int row=rowCount(parent);
		beginInsertRows(parent,row,row);
		evals[classRow-1].second.push_back(id);
		endInsertRows();
	} else {
		classRow=lastClassRow++;
		classRows.emplace(classId,classRow);
		auto vec=std::vector<EvalId>();
		vec.push_back(id);
		beginInsertRows(QModelIndex(),classRow,classRow);
		evals.emplace_back(eval.className(),vec);
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
	return _storage.eval(evals[classRow-1].second[evalRow]).name();
}

const AbstractEvaluator&  EvaluatorsTreeModel::eval(int classRow, int evalRow) const {
	if(classRow>0) {
		return _storage.eval(evalId(classRow,evalRow));
	} else {
		return *pendingEvals.at(evalRow);
	}
}
EvalId  EvaluatorsTreeModel::evalId(int classRow, int evalRow) const {
	return evals[classRow-1].second[evalRow];
}
EvalId EvaluatorsTreeModel::findEval(int classRow, const QString& evName) const {
	if(classRow<0) {
		return EvalId(0);
	}
	std::string name=evName.toStdString();
	return _storage.evalId(name);
}

const AbstractEvaluator &EvaluatorsTreeModel::eval(const EvaluatorsTreeItem &item) const {
	return eval(item.classRow(),item.evaluatorRow());
}

int EvaluatorsTreeModel::addClassRow(const EvalId &evId)
{
	const auto& eval=_storage.eval(evId);
	std::type_index classId=std::type_index(typeid(eval));
	auto it=classRows.find(classId);
	if(it!=classRows.end()) {
		size_t classRow=it->second;
		return classRow;
	} else {
		size_t classRow=lastClassRow++;
		classRows.emplace(classId,classRow);
		auto vec=std::vector<EvalId>();
		beginInsertRows(QModelIndex(),classRow,classRow);
		evals.emplace_back(eval.className(),vec);
		endInsertRows();
		return classRow;
	}
}

int EvaluatorsTreeModel::classRow(const EvalId &evId) const
{
	if(!_storage.isValid(evId)) {
		return -1;
	}
	std::type_index classId=std::type_index(typeid(_storage.eval(evId)));
	auto it=classRows.find(classId);
	if(it!=classRows.end()) {
		size_t classRow=it->second;
		return classRow;
	} else {
		return -1;
	}
}

int EvaluatorsTreeModel::evalRow(const EvalId &eval) const
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

QStringList EvaluatorsTreeModel::evalListByType(const EvalId &eval) const {
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
evalRowList(const QList<EvalId> &selected) const
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
/*
QVariantMap EvaluatorsTreeModel::propMap(const AbstractEvaluator &eval) const
{
	QVariantMap propMap;
	for(int p=0; p<eval.settingsCount(); ++p) {
		AbstractEvaluator::Setting opt=eval.setting(p);
		const QString& optName=opt.first;
		QVariant val;
		if(opt.second.userType()==evalType) {
			auto evid=opt.second.value<EvalId>();
			val=QString::fromStdString(_storage.eval(evid).name());

		} else if(opt.second.userType()==simulationType) {
			const auto& type=opt.second.value<Position::SimulationType>();
			val=QString::fromStdString(Position::simulationTypeName(type));
		} else if(opt.second.userType()==evalListType) {
			const auto& list=opt.second.value<QList<EvalId>>();
			QStringList evnames;
			for(const EvalId& evid:list) {
				const auto& ev=_storage.eval(evid);
				evnames<<QString::fromStdString(ev.name());
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
	MutableEvalPtr & ev=pendingEvals[evNum];
	for(int propRow=0; propRow<ev->settingsCount(); ++propRow) {
		QVariant oldVal=ev->settingValue(propRow);
		const QString& propName=ev->settingName(propRow);
		if(!propMap.contains(propName)) {
			continue;
		}
		const QVariant& propVal=propMap[propName];
		QVariant newVal;
		if(oldVal.userType()==evalType) {
			int cr=classRow(oldVal.value<EvalId>());
			newVal.setValue(findEval(cr,propVal.toString()));
		} else if(oldVal.userType()==simulationType) {
			auto simName=propVal.toString().toStdString();
			const auto& simType=Position::simulationType(simName);
			newVal.setValue(simType);
		} else if(oldVal.userType()==evalListType) {
			QList<EvalId> ptrs;
			int cr=classRow(oldVal.value<QList<EvalId>>().front());
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
*/
