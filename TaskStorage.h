#ifndef TASKSTORAGE_H
#define TASKSTORAGE_H
#include "AbstractCalcResult.h"
#include "FrameDescriptor.h"
#include "PterosSystemLoader.h"

#include <pteros/pteros.h>

#include <async++.h>

#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <unordered_set>
#include <cstdint>

#include <QObject>
#include <QVariant>
#include <readerwriterqueue/readerwriterqueue.h>

#include <libcuckoo/cuckoohash_map.hh>
template<typename K, typename V> using CuckooMap=cuckoohash_map<K, V, std::hash<K>>;
class AbstractEvaluator;
class EvaluatorPositionSimulation;
using EvalUPtr=std::unique_ptr<const AbstractEvaluator>;
template<class Tag, class Base = int> struct def_enum
{
    enum class type : Base { };
};
using EvalIdBase=uint_fast16_t;
using EvalId=def_enum<EvalUPtr,EvalIdBase>::type;
Q_DECLARE_METATYPE(EvalId)
Q_DECLARE_METATYPE(Eigen::Vector3d)
inline EvalId operator++(EvalId& id) {
	id=EvalId(static_cast<EvalIdBase>(id)+1);
	return id;
}

using CacheKey=std::pair<FrameDescriptor,EvalId>;
namespace std {
template <>
struct hash<CacheKey>
{
	size_t operator()(const CacheKey& k) const
	{
		size_t seed=hash<FrameDescriptor>()(k.first);
		hash_combine(seed,k.second);
		return seed;
	}
};
template <>
struct hash<EvalId>
{
	size_t operator()(const EvalId& k) const
	{
		return hash<EvalIdBase>()(static_cast<EvalIdBase>(k));
	}
};
}

class TaskStorage:public QObject
{
	Q_OBJECT
	friend class AbstractEvaluator;
	friend class EvaluatorsTreeModel;
	//TODO: specify functions instead;
public:
	TaskStorage();
	~TaskStorage();
	using CacheKey=::CacheKey;
	using Result=std::shared_ptr<AbstractCalcResult>;
	using Task=async::shared_task<Result>;
	using PterosSysTask=async::shared_task<pteros::System>;
	std::string getString(const FrameDescriptor &frame, const EvalId& evId, int col,
			      bool persistent=true) const;
	Result getResult(const FrameDescriptor &frame, const EvalId& evId) const;
	const PterosSysTask& getSysTask(const FrameDescriptor &frame) const;
	std::string getColumnName(const EvalId& id, int col) const;
	int getColumnCount(const EvalId& id) const;
	const AbstractEvaluator& eval(EvalId id) const
	{
		return *_evals.at(id);
	}
	const AbstractEvaluator& eval(const std::string& name) const
	{
		return eval(_evalNames.at(name));
	}
	EvalId evalId(const std::string& name) const
	{
		return _evalNames.at(name);
	}
	int sysTaskCount() const
	{
		return _sysCache.size();
	}
	int tasksRunningCount() const
	{
		return _tasksRunning;
	}
	int resultCount() const
	{
		return _results.size();
	}
	int tasksPendingCount() const
	{
		return _requests.size();
	}
	void evaluate(const FrameDescriptor& frame, const std::vector<EvalId>& evIds) const {
		for(const auto& evId:evIds) {
			getString(frame,evId,0);
		}
	}
	const std::unordered_map<EvalId,EvalUPtr>& evals() const {
		return _evals;
	}
	bool isStub(const EvalId& id) const {
		return id<=_maxStubEval;
	}
	bool isValid(const EvalId& id) const {
		return _evals.find(id)!=_evals.end();
	}
	void loadEvaluators(const QVariantMap& settings);
	bool ready() const {
		return _requests.empty();
	}
Q_SIGNALS:
	void evaluatorAdded(EvalId evId);
	void evaluatorIsGoingToBeRemoved(EvalId evId);
private:
	void removeResults(const EvalId& id) {
		//TODO:implement
	}

	EvalId addEvaluator(EvalUPtr evptr);
	void removeEvaluator(const EvalId& evId);
	const Task& getTask(const CacheKey &key,
				    bool persistent) const;
	const Task& getTask(const FrameDescriptor &frame, const EvalId &evId,
				    bool persistent) const
	{
		return getTask(CacheKey(frame,evId),persistent);
	}
	const Task& makeTask(const CacheKey &key,
			    bool persistent) const;
	inline void pushTask(const CacheKey &key) const
	{
		auto& oldK=_tasksRingBuf[_tasksRBpos];
		_tasks.erase(oldK);
		oldK=key;
		_tasksRBpos=(_tasksRBpos+1)%_tasksRingBufSize;
	}
	//must only run in worker thread
	void runRequests() const
	{
		static auto tid=std::this_thread::get_id();
		assert(tid==std::this_thread::get_id());
		CacheKey key;
		while(_tasksRunning<_maxRunningCount)//consume
		{
			if(_requestQueue.try_dequeue(key)) {
				getTask(key,true);
			} else {
				_requests.reserve(0);
				return;
			}
		}
	}

private:
	const int evalType=QVariant::fromValue(EvalId()).userType();
	const int simulationType;
	const int evalListType=QVariant::fromValue(QList<EvalId>()).userType();
	using Vector3d=Eigen::Vector3d;
	const int vec3dType=QVariant::fromValue(Vector3d()).userType();

	using MutableEvalPtr=std::unique_ptr<AbstractEvaluator>;
	MutableEvalPtr makeEvaluator(int typeNum) const;
	QStringList supportedTypes() const;
	std::string evalTypeName(int typeNum) const;
	void setEval(MutableEvalPtr & ev, const QVariantMap &propMap);



	mutable CuckooMap<CacheKey,bool> _requests;
	using RWQueue=moodycamel::ReaderWriterQueue<CacheKey>;
	mutable RWQueue _requestQueue;

	mutable CuckooMap<CacheKey,Result> _results;

	const int _minRunningCount=(std::thread::hardware_concurrency()+1)*2;
	const int _maxRunningCount=_minRunningCount*2;
	mutable std::unordered_map<CacheKey,Task> _tasks;
	mutable std::atomic<int> _tasksRunning{0};
	const size_t _tasksRingBufSize=_maxRunningCount*3;
	mutable std::vector<CacheKey> _tasksRingBuf;
	mutable size_t _tasksRBpos=0;
	mutable async::threadpool_scheduler workerPool{1};

	mutable std::unordered_map<FrameDescriptor,PterosSysTask> _sysCache;
	const size_t _sysRingBufSize=_maxRunningCount;
	mutable std::vector<FrameDescriptor> _sysRingBuf;
	mutable size_t sysRingBufIndex=0;
	PterosSystemLoader _systemLoader;

	std::unordered_map<std::string,EvalId> _evalNames;
	std::unordered_map<EvalId,EvalUPtr> _evals;
	EvalId _currentId;
	EvalId _maxStubEval;
public:
	//TODO: make theese const
	EvalId evaluatorPositionSimulation;
	EvalId evaluatorDistance;
	EvalId evaluatorChi2;
	EvalId evaluatorTrasformationMatrix;
	EvalId evaluatorEulerAngle;
};

#endif // TASKSTORAGE_H
