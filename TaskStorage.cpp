#include "TaskStorage.h"
#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
#include "EvaluatorChi2.h"
#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"

#include "AV/Position.h"

const int TaskStorage::evalType=QVariant::fromValue(EvalId()).userType();
const int TaskStorage::simulationType=QVariant::fromValue(Position::SimulationType()).userType();
const int TaskStorage::evalListType=QVariant::fromValue(QList<EvalId>()).userType();
const int TaskStorage::vec3dType=QVariant::fromValue(TaskStorage::Vector3d()).userType();


TaskStorage::TaskStorage():_tasksRingBuf(_tasksRingBufSize),
	_sysRingBuf(_sysRingBufSize),_currentId(EvalId(0))
{
	addEvaluator(std::make_unique<const EvaluatorPositionSimulation>(*this,"unknown"));
	evaluatorPositionSimulation=_currentId;
	addEvaluator(std::make_unique<const EvaluatorDistance>(*this,"unknown"));
	evaluatorDistance=_currentId;
	addEvaluator(std::make_unique<const EvaluatorChi2>(*this,"unknown"));
	evaluatorChi2=_currentId;
	addEvaluator(std::make_unique<const EvaluatorTrasformationMatrix>(*this,"unknown"));
	evaluatorTrasformationMatrix=_currentId;
	addEvaluator(std::make_unique<const EvaluatorEulerAngle>(*this,"unknown"));
	evaluatorEulerAngle=_currentId;
	_maxStubEval=_currentId;
}

TaskStorage::~TaskStorage()
{
}

//must only run in worker thread
const TaskStorage::Task &
TaskStorage::getTask(const CacheKey &key, bool persistent) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());

	//check in tasks
	auto it=_tasks.find(key);
	if(it!=_tasks.end()) {
		return it->second;
	} else {
		//check in results
		Result res;
		bool exists=_results.find(key,res);
		if(exists) {
			Task& task=_tasks.emplace(key,async::make_task(res).share()).first->second;
			pushTask(key);
			return task;
		}
	}
	return makeTask(key,persistent);
}
//must only run in worker thread
const TaskStorage::Task &TaskStorage::makeTask(const CacheKey &key, bool persistent) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());
	//append a new job
	Task& task=_tasks.emplace(key,eval(key.second).makeTask(key.first)).first->second;
	pushTask(key);
	_tasksRunning++;

	task.then([this,key,persistent](Task tres){
		assert(tres.valid());
		if(persistent) {
			try{
				_results.insert(key,tres.get());
			}
			catch (...) {
				std::cerr<<"ERROR! Task is invalid: "
					   +key.first.fullName()+" "
					   +eval(key.second).name()<<std::flush;
			}
		}
		_requests.erase(key);
		_tasksRunning--;
		if(_tasksRunning<_minRunningCount) {
			async::spawn(workerPool,[this]{
				runRequests();
			});
		}
	});
	return task;
}

QVariantMap TaskStorage::propMap(const AbstractEvaluator &ev) const
{
	QVariantMap propMap;
	for(int p=0; p<ev.settingsCount(); ++p) {
		AbstractEvaluator::Setting opt=ev.setting(p);
		const QString& optName=opt.first;
		QVariant val;
		if(opt.second.userType()==evalType) {
			auto evid=opt.second.value<EvalId>();
			val=QString::fromStdString(eval(evid).name());

		} else if(opt.second.userType()==simulationType) {
			const auto& type=opt.second.value<Position::SimulationType>();
			val=QString::fromStdString(Position::simulationTypeName(type));
		} else if(opt.second.userType()==evalListType) {
			const auto& list=opt.second.value<QList<EvalId>>();
			QStringList evnames;
			for(const EvalId& evid:list) {
				const auto& ev=eval(evid);
				evnames<<QString::fromStdString(ev.name());
			}
			val=evnames;
		} else if(opt.second.userType()==vec3dType) {
			const Vector3d& vec=opt.second.value<Vector3d>();
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

std::vector<TaskStorage::MutableEvalPtr>
TaskStorage::loadEvaluators(const QVariantMap &settings) {
	static const QStringList classOrder{supportedTypes()};
	std::vector<MutableEvalPtr> drafts;
	for(int curEvalType=0; curEvalType<classOrder.size(); curEvalType++) {
		const QString& className=classOrder.at(curEvalType);
		const QVariantMap& evals=settings[className].toMap();
		for(auto i=evals.constBegin();i!=evals.constEnd(); ++i) {
			const QVariantMap& propMap=i.value().toMap();
			auto ev=makeEvaluator(curEvalType);
			const QString& evName=i.key();
			ev->setName(evName.toStdString());
			setEval(ev,propMap);
			if(propMap["isDraft"]==true) {
				drafts.push_back(std::move(ev));
			} else {
				addEvaluator(std::move(ev));
			}
		}
	}
	return drafts;
}

TaskStorage::MutableEvalPtr TaskStorage::makeEvaluator(int typeNum) const {
	switch (typeNum)
	{
	case 0:
		return std::make_unique<EvaluatorPositionSimulation>(*this,"new LP");
	case 1:
		return std::make_unique<EvaluatorDistance>(*this,"new dist");
	case 2:
		return std::make_unique<EvaluatorChi2>(*this,"new χ²");
	case 3:
		return std::make_unique<EvaluatorTrasformationMatrix>(*this,"new coordinate system");
	case 4:
		return std::make_unique<EvaluatorEulerAngle>(*this,"new euler angles");
	default:
		return MutableEvalPtr();
	}
}

QStringList TaskStorage::supportedTypes() const {
	static QStringList vec;
	if (vec.size()==0) {
		int i;
		std::string typeName;
		for (i=0, typeName=evalTypeName(i);
		     !typeName.empty();
		     ++i,typeName=evalTypeName(i)){
			vec<<QString::fromStdString(typeName);
		}
	}
	return vec;
}

std::string TaskStorage::evalTypeName(int typeNum) const
{
	const MutableEvalPtr& eval=makeEvaluator(typeNum);
	if(eval) {
		return eval->className();
	}
	return "";
}

void TaskStorage::setEval(TaskStorage::MutableEvalPtr &ev, const QVariantMap &propMap) const {
	for(int propRow=0; propRow<ev->settingsCount(); ++propRow) {
		QVariant oldVal=ev->settingValue(propRow);
		const QString& propName=ev->settingName(propRow);
		if(!propMap.contains(propName)) {
			continue;
		}
		const QVariant& propVal=propMap[propName];
		QVariant newVal;
		if(oldVal.userType()==evalType) {
			newVal.setValue(evalId(propVal.toString().toStdString()));
		} else if(oldVal.userType()==simulationType) {
			auto simName=propVal.toString().toStdString();
			const auto& simType=Position::simulationType(simName);
			newVal.setValue(simType);
		} else if(oldVal.userType()==evalListType) {
			QList<EvalId> ptrs;
			QStringList names=propVal.toStringList();
			for(const QString& name:names) {
				ptrs.append(evalId(name.toStdString()));
			}
			newVal.setValue(ptrs);
		} else if(oldVal.userType()==vec3dType) {
			Eigen::Vector3d vec;
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

//must only run in worker thread
const TaskStorage::PterosSysTask &TaskStorage::getSysTask(const FrameDescriptor &frame) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());
	auto it=_sysCache.find(frame);
	if(it!=_sysCache.end()) {
		return it->second;
	}
	else {
		auto &oldKey=_sysRingBuf[sysRingBufIndex];
		_sysCache.erase(oldKey);
		oldKey=frame;
		++sysRingBufIndex;
		sysRingBufIndex%=_sysRingBufSize;
		auto pair=_sysCache.emplace(frame,_systemLoader.makeTask(frame));
		return pair.first->second;
	}
}

std::string TaskStorage::getColumnName(const EvalId &id, int col) const
{
	return eval(id).columnName(col);
}

EvalId TaskStorage::addEvaluator(EvalUPtr evptr)
{
	_evals.emplace(++_currentId,std::move(evptr));
	_evalNames.emplace(eval(_currentId).name(),_currentId);
	Q_EMIT evaluatorAdded(_currentId);
	return _currentId;
}

void TaskStorage::removeEvaluator(const EvalId &evId) {
	Q_EMIT evaluatorIsGoingToBeRemoved(evId);
	_evalNames.erase(eval(evId).name());
	_evals.erase(evId);
	removeResults(evId);
}

//must only run in the main thread;
std::string TaskStorage::getString(const FrameDescriptor &frame,
				   const EvalId &evId, int col,
				   bool persistent) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());
	auto key=CacheKey(frame,evId);
	Result result;
	bool ready=_results.find(key,result);
	if(ready) {
		return result->toString(col);
	} else {
		_requestQueue.enqueue(key);//produce
		_requests.insert(key,persistent);
		if(_requestQueue.size_approx()==1) {
			async::spawn(workerPool,[this]{
				runRequests();
			});
		}
	}
	return "...";
}

TaskStorage::Result TaskStorage::getResult(const FrameDescriptor &frame, const EvalId &evId) const
{
	auto key=CacheKey(frame,evId);
	Result result;
	_results.find(key,result);
	return result;
}
