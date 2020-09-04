#include "TaskStorage.h"
#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorFretEfficiency.h"
#include "EvaluatorDistance.h"
#include "EvaluatorDistanceDistribution.h"
#include "EvaluatorChi2.h"
#include "EvaluatorChi2Contribution.h"
#include "EvaluatorChi2r.h"
#include "EvaluatorWeightedResidual.h"
//#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"
#include "EvaluatorMinDistance.h"
#include "EvaluatorSphereAVOverlap.h"
#include "EvaluatorAvFile.h"
#include "EvaluatorAvVolume.h"

#include "AV/Position.h"
#include "CalcResult.h"
#include "split_string.h"

#include <QFileInfo>

const int TaskStorage::evalType = QVariant::fromValue(EvalId()).userType();
const int TaskStorage::simulationType =
	QVariant::fromValue(Position::SimulationType()).userType();
const int TaskStorage::evalListType =
	QVariant::fromValue(QList<EvalId>()).userType();
const int TaskStorage::vec3dType =
	QVariant::fromValue(TaskStorage::Vector3d()).userType();


TaskStorage::TaskStorage()
    : _tasksRingBuf(_tasksRingBufSize), _currentId(EvalId(0))
{
	addEvaluator(std::make_unique<const EvaluatorPositionSimulation>(
		*this, "unknown"));
	evaluatorPositionSimulation = _currentId;
	addEvaluator(std::make_unique<const EvaluatorAvFile>(*this, "unknown"));
	evaluatorEvaluatorAvFile = _currentId;
	addEvaluator(
		std::make_unique<const EvaluatorAvVolume>(*this, "unknown"));
	evaluatorEvaluatorAvVolume = _currentId;
	addEvaluator(std::make_unique<const EvaluatorFretEfficiency>(
		*this, "unknown"));
	evaluatorFretEfficiency = _currentId;
	addEvaluator(
		std::make_unique<const EvaluatorDistance>(*this, "unknown"));
	evaluatorDistance = _currentId;
	addEvaluator(std::make_unique<const EvaluatorChi2>(*this, "unknown"));
	evaluatorChi2 = _currentId;
	addEvaluator(std::make_unique<const EvaluatorChi2r>(*this, "unknown"));
	evaluatorChi2r = _currentId;
	addEvaluator(std::make_unique<const EvaluatorWeightedResidual>(
		*this, "unknown"));
	evaluatorWeightedResidual = _currentId;
	addEvaluator(
		std::make_unique<const EvaluatorMinDistance>(*this, "unknown"));
	evaluatorMinDistance = _currentId;
	addEvaluator(std::make_unique<const EvaluatorSphereAVOverlap>(
		*this, "unknown"));
	evaluatorAVOverlap = _currentId;
	addEvaluator(std::make_unique<const EvaluatorChi2Contribution>(
		*this, "unknown"));
	evaluatorChi2Conribution = _currentId;
	addEvaluator(std::make_unique<const EvaluatorTrasformationMatrix>(
		*this, "unknown"));
	evaluatorTrasformationMatrix = _currentId;
	addEvaluator(
		std::make_unique<const EvaluatorEulerAngle>(*this, "unknown"));
	addEvaluator(std::make_unique<const EvaluatorDistanceDistribution>(
		*this, "unknown"));
	evaluatorEulerAngle = _currentId;

	_maxStubEval = _currentId;
	async::spawn(_runRequestsThread, [this] { runRequests(); });
}

TaskStorage::~TaskStorage()
{
	_runRequests.clear();
	while (tasksRunningCount()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}

// must only run in worker thread
const TaskStorage::Task &TaskStorage::getTask(const CacheKey &key,
					      bool persistent) const
{
	static auto tid = std::this_thread::get_id();
	assert(tid == std::this_thread::get_id());
	// check Eval validity
	if (!isValid(key.second)) {
		Task &task =
			_tasks.emplace(key, async::make_task(Result()).share())
				.first->second;
		pushTask(key);
		return task;
	}

	// check in tasks
	auto it = _tasks.find(key);
	if (it != _tasks.end()) {
		return it->second;
	} else {
		// check in results
		Result res;
		bool exists = _results.find(key, res);
		if (exists) {
			Task &task =
				_tasks.emplace(key,
					       async::make_task(res).share())
					.first->second;
			pushTask(key);
			return task;
		}
	}
	return makeTask(key, persistent);
}
// must only run in worker thread
const TaskStorage::Task &TaskStorage::makeTask(const CacheKey &key,
					       bool persistent) const
{
	static auto tid = std::this_thread::get_id();
	assert(tid == std::this_thread::get_id());
	// append a new job
	Task &task = _tasks.emplace(key, eval(key.second).makeTask(key.first))
			     .first->second;
	pushTask(key);
	_tasksRunning++;

	task.then([this, key, persistent](Task tres) {
		assert(tres.valid());
		if (persistent) {
			try {
				_results.insert(key, tres.get());
			} catch (...) {
				std::cerr
					<< "ERROR! Task is invalid (exception): "
						   + key.first.fullName() + " "
						   + eval(key.second).name()
					<< std::flush;
			}
		}
		_requests.erase(key);
		_tasksRunning--;
	});
	return task;
}

void TaskStorage::runRequests() const
{
	static auto tid = std::this_thread::get_id();
	assert(tid == std::this_thread::get_id());
	_runRequests.test_and_set();
	unsigned waitms = 20u;
	while (_runRequests.test_and_set()) {

		while (_pauseCount) {
			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		}
		if (_tasksRunning < _minRunningCount
		    && tasksPendingCount() > 0) {
			if (_tasksRunning < _minRunningCount / 2) {
				if (waitms < 20u) {
					std::cout<<"Number of tasks running ("
						   +std::to_string(_tasksRunning)+
						   ") is too low, this might "
						   "lead to performance decrease. "
						   "(waitms="
						   +std::to_string(waitms)
						   +")\n"<<std::flush;
				}
				waitms = 0;
			}
			waitms = 1 + waitms / 2;
		} else {
			waitms = std::min(20u, ++waitms);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(waitms));
		}
		CacheKey key;
		while (_tasksRunning < _maxRunningCount) // consume
		{
			if (_requestQueue.try_dequeue(key)) {
				getTask(key, true);
			} else {
				_requests.reserve(64);
				waitms = 20u;
				/*auto locked=_requests.lock_table();
				std::string str;
				for(auto pair:locked) {
					const CacheKey &k=pair.first;
					str+=k.first.fullName()+", "
					     +std::to_string(int(k.second))+"\n";
				}
				if(str.size()) {
					std::cout<<"Residual requests: \n"+str
						<<std::flush;
				}*/
				break;
			}
		}
	}
}

QVariantMap TaskStorage::propMap(const AbstractEvaluator &ev) const
{
	QVariantMap propMap;
	for (int p = 0; p < ev.settingsCount(); ++p) {
		AbstractEvaluator::Setting opt = ev.setting(p);
		const QString &optName = opt.first;
		QVariant val;
		if (opt.second.userType() == evalType) {
			auto evid = opt.second.value<EvalId>();
			val = QString::fromStdString(eval(evid).name());

		} else if (opt.second.userType() == simulationType) {
			const auto &type =
				opt.second.value<Position::SimulationType>();
			val = QString::fromStdString(
				Position::simulationTypeName(type));
		} else if (opt.second.userType() == evalListType) {
			const auto &list = opt.second.value<QList<EvalId>>();
			QStringList evnames;
			for (const EvalId &evid : list) {
				const auto &ev = eval(evid);
				evnames << QString::fromStdString(ev.name());
			}
			val = evnames;
		} else if (opt.second.userType() == vec3dType) {
			const Vector3d &vec = opt.second.value<Vector3d>();
			QVariantList list;
			for (int i : {0, 1, 2}) {
				list.push_back(vec[i]);
			}
			val = list;
		} else {
			val = opt.second;
		}
		propMap[optName] = val;
	}
	return propMap;
}

std::vector<TaskStorage::MutableEvalPtr>
TaskStorage::loadEvaluators(const QVariantMap &settings)
{
	static const QStringList classOrder{supportedTypes()};
	std::vector<MutableEvalPtr> drafts;
	for (int curEvalType = 0; curEvalType < classOrder.size();
	     curEvalType++) {
		const QString &className = classOrder.at(curEvalType);
		const QVariantMap &evals = settings[className].toMap();
		for (auto i = evals.constBegin(); i != evals.constEnd(); ++i) {
			const QVariantMap &propMap = i.value().toMap();
			auto ev = makeEvaluator(curEvalType);
			const QString &evName = i.key();
			ev->setName(evName.toStdString());
			setEval(ev, propMap);
			if (propMap["isDraft"] == true) {
				drafts.push_back(std::move(ev));
			} else {
				addEvaluator(std::move(ev));
			}
		}
	}
	return drafts;
}

TaskStorage::MutableEvalPtr TaskStorage::makeEvaluator(int typeNum) const
{
	switch (typeNum) {
	case 0:
		return std::make_unique<EvaluatorPositionSimulation>(*this,
								     "new LP");
	case 1:
		return std::make_unique<EvaluatorAvFile>(*this, "new AV File");
	case 2:
		return std::make_unique<EvaluatorAvVolume>(*this,
							   "new AV Volume");
	case 3:
		return std::make_unique<EvaluatorFretEfficiency>(*this,
								 "new <E>");
	case 4:
		return std::make_unique<EvaluatorDistance>(*this,
							   "new distance");
	case 5:
		return std::make_unique<EvaluatorWeightedResidual>(
			*this, "new w.res.");
	case 6:
		return std::make_unique<EvaluatorChi2Contribution>(
			*this, "new χ² contribution");
	case 7:
		return std::make_unique<EvaluatorMinDistance>(
			*this, "new minimum distance");
	case 8:
		return std::make_unique<EvaluatorSphereAVOverlap>(
			*this, "new AV overlap");
	case 9:
		return std::make_unique<EvaluatorChi2>(*this, "new χ²");
	case 10:
		return std::make_unique<EvaluatorChi2r>(*this, "new χᵣ²");
	case 11:
		return std::make_unique<EvaluatorTrasformationMatrix>(
			*this, "new coordinate system");
	case 12:
		return std::make_unique<EvaluatorEulerAngle>(
			*this, "new euler angles");
	case 13:
		return std::make_unique<EvaluatorDistanceDistribution>(
			*this, "new distance distribution");
	default:
		return MutableEvalPtr();
	}
}

QStringList TaskStorage::supportedTypes() const
{
	static QStringList vec;
	if (vec.size() == 0) {
		int i;
		std::string typeName;
		for (i = 0, typeName = evalTypeName(i); !typeName.empty();
		     ++i, typeName = evalTypeName(i)) {
			vec << QString::fromStdString(typeName);
		}
	}
	return vec;
}

std::string TaskStorage::evalTypeName(int typeNum) const
{
	const MutableEvalPtr &eval = makeEvaluator(typeNum);
	if (eval) {
		return eval->className();
	}
	return "";
}

void TaskStorage::setEval(TaskStorage::MutableEvalPtr &ev,
			  const QVariantMap &propMap) const
{
	for (int propRow = 0; propRow < ev->settingsCount(); ++propRow) {
		QVariant oldVal = ev->settingValue(propRow);
		const QString &propName = ev->settingName(propRow);
		if (!propMap.contains(propName)) {
			continue;
		}
		const QVariant &propVal = propMap[propName];
		QVariant newVal;
		if (oldVal.userType() == evalType) {
			newVal.setValue(
				evalId(propVal.toString().toStdString()));
		} else if (oldVal.userType() == simulationType) {
			auto simName = propVal.toString().toStdString();
			const auto &simType = Position::simulationType(simName);
			newVal.setValue(simType);
		} else if (oldVal.userType() == evalListType) {
			QList<EvalId> ptrs;
			QStringList names = propVal.toStringList();
			for (const QString &name : names) {
				auto id = evalId(name.toStdString());
				if (isValid(id)) {
					ptrs.append(id);
				} else {
					std::cerr
						<< "Error in JSON file! "
						   "Can not find evaluator "
							   + name.toStdString()
							   + "\n"
						<< std::flush;
				}
			}
			newVal.setValue(ptrs);
		} else if (oldVal.userType() == vec3dType) {
			Eigen::Vector3d vec;
			const QVariantList &list = propVal.toList();
			for (int i : {0, 1, 2}) {
				vec[i] = list[i].toDouble();
			}
			newVal.setValue(vec);
		} else {
			newVal = propVal;
		}
		ev->setSetting(propRow, newVal);
	}
}

async::task<int> TaskStorage::numFrames(const std::string &topPath,
					const std::string &trajPath) const
{
	return _systemLoader.numFrames(topPath, trajPath);
}

std::string TaskStorage::getColumnName(const EvalId &id, int col) const
{
	return eval(id).columnName(col);
}

std::string TaskStorage::evalName(const EvalId &id) const
{
	return eval(id).name();
}

std::unordered_map<std::string, std::string>
TaskStorage::getStrings(const FrameDescriptor &frame) const
{
	using std::string;
	std::unordered_map<string, string> res;
	for (const auto &pair : _evals) {
		if (isStub(pair.first)) {
			continue;
		}
		const AbstractEvaluator &ev = *pair.second;
		for (int c = 0; c < ev.columnCount(); ++c) {
			res[ev.columnName(c)] = getString(frame, pair.first, c);
		}
	}
	return res;
}

void TaskStorage::evaluate(const FrameDescriptor &frame) const
{
	for (const auto &pair : _evals) {
		if (isStub(pair.first)) {
			continue;
		}
		if (pair.second->columnCount() > 0) {
			getString(frame, pair.first, 0);
		}
	}
}

EvalId TaskStorage::addEvaluator(EvalUPtr evptr)
{
	_evals.emplace(++_currentId, std::move(evptr));
	_evalNames.emplace(eval(_currentId).name(), _currentId);
	Q_EMIT evaluatorAdded(_currentId);
	_tasksRingBufSize = std::max(_tasksRingBufSize, _evals.size() * 2);
	if (_tasksRingBufSize > _tasksRingBuf.size()) {
		_tasksRingBuf.resize(_tasksRingBufSize);
	}
	return _currentId;
}

void TaskStorage::removeEvaluator(const EvalId &evId)
{
	Q_EMIT evaluatorIsGoingToBeRemoved(evId);
	_evalNames.erase(eval(evId).name());
	const auto it = _evals.find(evId);
	_removedEvals.push_back(std::move(it->second));
	_evals.erase(it);
	removeResults(evId);
}

// must only run in the main thread;
std::string TaskStorage::getString(const FrameDescriptor &frame,
				   const EvalId &evId, int col,
				   bool persistent) const
{
	static auto tid = std::this_thread::get_id();
	assert(tid == std::this_thread::get_id());
	auto key = CacheKey(frame, evId);
	Result result;
	bool ready = _results.find(key, result);
	if (ready) {
		return result->toString(col);
	} else {
		_requestQueue.enqueue(key); // produce
		_requests.insert(key, persistent);
	}
	return "...";
}

TaskStorage::Result TaskStorage::getResult(const FrameDescriptor &frame,
					   const EvalId &evId) const
{
	auto key = CacheKey(frame, evId);
	Result result;
	_results.find(key, result);
	return result;
}

void TaskStorage::setResults(const std::string &fName,
			     const std::vector<FrameDescriptor> &frames)
{
	using std::string;
	using std::vector;

	auto p = pause();
	std::ifstream infile;

	infile.open(fName, std::ifstream::in);
	if (!infile.is_open()) {
		std::cerr << "could not open the file: " + fName + "\n"
			  << std::flush;
		return;
	}

	string str;
	getline(infile, str);
	vector<string> evNames = split(str, '\t');
	evNames.erase(evNames.begin());
	vector<EvalId> evIds;
	for (const auto &name : evNames) {
		evIds.push_back(evalId(name));
	}

	std::unordered_map<string, FrameDescriptor> frameMap;
	for (const FrameDescriptor &frame : frames) {
		QString shortName;
		shortName = QString::fromStdString(frame.trajFileName());
		shortName = QFileInfo(shortName).fileName() + " "
			    + QString::number(frame.frame());
		frameMap[shortName.toStdString()] = frame;
	}

	while (getline(infile, str)) {
		str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
		vector<string> values = split(str, '\t');
		if (values.size() == 0) {
			continue;
		}
		if (frameMap.count(values[0]) == 0) {
			std::cout << "Not found: <" + values[0] + ">\n"
				  << std::flush;
			continue;
		}
		FrameDescriptor frame = frameMap.at(values[0]);
		for (int i = 1; i < values.size(); ++i) {
			if (values[i].empty()) {
				continue;
			}
			const EvalId evId = evIds[i - 1];
			CacheKey key = CacheKey(frame, evId);
			double val = -9999.9;
			try {
				val = std::atof(values[i].c_str());
			} catch (...) {
				std::cerr
					<< "ERROR! float conversion failed!\n";
				return;
			}
			Result res = std::make_shared<CalcResult<double>>(val);
			_results.insert(key, res);
			_requests.erase(key);
		}
	}
}
