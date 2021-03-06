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
#include <algorithm>

#include <QObject>
#include <QVariant>
#include <readerwriterqueue/readerwriterqueue.h>

#include <libcuckoo/cuckoohash_map.hh>

// TODO: remove this. Added for compatibility with older versions of libcuckoo.
namespace libcuckoo
{
}
using namespace libcuckoo;
template <typename K, typename V>
using CuckooMap = cuckoohash_map<K, V, std::hash<K>>;
class AbstractEvaluator;
class EvaluatorPositionSimulation;
using EvalUPtr = std::unique_ptr<const AbstractEvaluator>;
template <class Tag, class Base = int> struct def_enum {
	enum class type : Base {};
};
using EvalIdBase = uint_fast16_t;
using EvalId = def_enum<EvalUPtr, EvalIdBase>::type;
Q_DECLARE_METATYPE(EvalId)
Q_DECLARE_METATYPE(Eigen::Vector3d)
inline EvalId operator++(EvalId &id)
{
	id = EvalId(static_cast<EvalIdBase>(id) + 1);
	return id;
}

using CacheKey = std::pair<FrameDescriptor, EvalId>;
namespace std
{
template <> struct hash<CacheKey> {
	size_t operator()(const CacheKey &k) const
	{
		size_t seed = hash<FrameDescriptor>()(k.first);
		hash_combine(seed, k.second);
		return seed;
	}
};
template <> struct hash<EvalId> {
	size_t operator()(const EvalId &k) const
	{
		return hash<EvalIdBase>()(static_cast<EvalIdBase>(k));
	}
};
} // namespace std

class TaskStorage : public QObject
{
	Q_OBJECT
	friend class AbstractEvaluator;
	friend class EvaluatorsTreeModel;
	// TODO: specify functions instead;

	mutable std::atomic<int> _pauseCount{0};

public:
	class Pause
	{
		friend class TaskStorage;

	public:
		const TaskStorage &_storage;
		Pause(const TaskStorage &storage) : _storage(storage)
		{
			++_storage._pauseCount;
		}
		~Pause()
		{
			--_storage._pauseCount;
		}
	};
	TaskStorage();
	~TaskStorage();
	using CacheKey = ::CacheKey;
	using Result = std::shared_ptr<AbstractCalcResult>;
	using Task = async::shared_task<Result>;
	using PterosSysTask = async::shared_task<pteros::System>;
	std::string getString(const FrameDescriptor &frame, const EvalId &evId,
			      int col, bool persistent = true) const;
	Result getResult(const FrameDescriptor &frame,
			 const EvalId &evId) const;
	void setResults(const std::string &fName,
			const std::vector<FrameDescriptor> &frames);
	const PterosSysTask getSysTask(const FrameDescriptor &frame) const
	{
		return _systemLoader.getTask(frame);
	}
	async::task<int> numFrames(const std::string &topPath,
				   const std::string &trajPath) const;
	std::string getColumnName(const EvalId &id, int col) const;
	const AbstractEvaluator &eval(EvalId id) const
	{
		return *_evals.at(id);
	}
	template <typename T> std::vector<EvalId> evalIds() const
	{
		std::vector<EvalId> list;
		// T eval(_storage,"none");
		for (const auto &pair : _evals) {

			if (isStub(pair.first)) {
				continue;
			}
			if (dynamic_cast<const T *>(pair.second.get())) {
				list.push_back(pair.first);
			}
		}
		return list;
	}
	std::string evalName(const EvalId &id) const;
	const AbstractEvaluator &eval(const std::string &name) const
	{
		static auto tid = std::this_thread::get_id();
		assert(tid == std::this_thread::get_id());
		return eval(_evalNames.at(name));
	}
	EvalId evalId(const std::string &name) const
	{
		static auto tid = std::this_thread::get_id();
		assert(tid == std::this_thread::get_id());
		if (_evalNames.count(name) == 0) {
			return EvalId(-1);
		} else {
			return _evalNames.at(name);
		}
	}
	int sysTaskCount() const
	{
		return _systemLoader.taskCount();
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
	void evaluate(const FrameDescriptor &frame,
		      const std::vector<EvalId> &evIds) const
	{
		for (const auto &evId : evIds) {
			getString(frame, evId, 0);
		}
	}
	std::unordered_map<std::string, std::string>
	getStrings(const FrameDescriptor &frame) const;
	void evaluate(const FrameDescriptor &frame) const;
	const std::unordered_map<EvalId, EvalUPtr> &evals() const
	{
		static auto tid = std::this_thread::get_id();
		assert(tid == std::this_thread::get_id());
		return _evals;
	}
	bool isStub(const EvalId &id) const
	{
		return id <= _maxStubEval;
	}
	bool isValid(const EvalId &id) const
	{
		return _evals.find(id) != _evals.end();
	}
	using MutableEvalPtr = std::unique_ptr<AbstractEvaluator>;
	std::string evalTypeName(int typeNum) const;
	MutableEvalPtr makeEvaluator(int typeNum) const;
	template <typename T> MutableEvalPtr makeEvaluator() const
	{
		T tmp(*this, "none");
		return std::make_unique<T>(*this, "new " + tmp.className());
	}
	QStringList supportedTypes() const;
	std::vector<MutableEvalPtr> loadEvaluators(const QVariantMap &settings);
	bool ready() const
	{
		return !(tasksPendingCount() + tasksRunningCount());
	}
	Pause pause() const
	{
		return Pause(*this);
	}
	std::unique_ptr<Pause> pausePtr() const
	{
		return std::make_unique<Pause>(*this);
	}
	std::string bufferStats() const
	{
		std::string sz;
		sz += "_requests:\n" + cuckooMapStats(_requests);
		sz += "\n_results:\n" + cuckooMapStats(_results);
		sz += "\n_tasks:\n" + uoMapStats(_tasks);
		sz += "\n_tasksRingBuf\n" + vectorStats(_tasksRingBuf);
		sz += "\n_requestQueue.size_approx() = "
		      + std::to_string(_requestQueue.size_approx());
		return sz;
	}

Q_SIGNALS:
	void evaluatorAdded(EvalId evId);
	void evaluatorIsGoingToBeRemoved(EvalId evId);

private:
	template <typename T> static std::string cuckooMapStats(const T &map)
	{
		std::string sz;
		using std::to_string;
		sz += "size() = " + to_string(map.size()) + "\n";
		sz += "bucket_count() = " + to_string(map.bucket_count())
		      + "\n";
		sz += "load_factor() = " + to_string(map.load_factor()) + "\n";
		sz += "capacity() = " + to_string(map.capacity()) + "\n";
		sz += "slot_per_bucket() = " + to_string(map.slot_per_bucket())
		      + "\n";
		return sz;
	}
	template <typename T> static std::string uoMapStats(const T &map)
	{
		std::string sz;
		using std::to_string;
		sz += "size() = " + to_string(map.size()) + "\n";
		sz += "bucket_count() = " + to_string(map.bucket_count())
		      + "\n";
		sz += "load_factor() = " + to_string(map.load_factor()) + "\n";
		return sz;
	}
	template <typename T> static std::string vectorStats(const T &vec)
	{
		std::string sz;
		using std::to_string;
		sz += "size() = " + to_string(vec.size()) + "\n";
		sz += "capacity() = " + to_string(vec.capacity()) + "\n";
		return sz;
	}
	void removeResults(const EvalId &id)
	{
		// TODO:implement
	}

	EvalId addEvaluator(EvalUPtr evptr);
	void removeEvaluator(const EvalId &evId);
	const Task &getTask(const CacheKey &key, bool persistent) const;
	const Task &getTask(const FrameDescriptor &frame, const EvalId &evId,
			    bool persistent) const
	{
		return getTask(CacheKey(frame, evId), persistent);
	}
	const Task &makeTask(const CacheKey &key, bool persistent) const;
	inline void pushTask(const CacheKey &key) const
	{
		auto &oldK = _tasksRingBuf[_tasksRBpos];
		_tasks.erase(oldK);
		oldK = key;
		_tasksRBpos = (_tasksRBpos + 1) % _tasksRingBufSize;
	}
	// must only run in worker thread
	void runRequests() const;


private:
	mutable std::atomic_flag _runRequests = ATOMIC_FLAG_INIT;
	async::threadpool_scheduler _runRequestsThread{1};
	async::task<void> _runRequestsTask;

	static const int evalType;
	static const int simulationType;
	static const int evalListType;
	using Vector3d = Eigen::Vector3d;
	static const int vec3dType;

	QVariantMap propMap(const AbstractEvaluator &ev) const;
	void setEval(MutableEvalPtr &ev, const QVariantMap &propMap) const;
	QVariantMap evalSettings(const AbstractEvaluator &eval) const;

	mutable CuckooMap<CacheKey, bool> _requests;
	using RWQueue = moodycamel::ReaderWriterQueue<CacheKey>;
	mutable RWQueue _requestQueue;

	mutable CuckooMap<CacheKey, Result> _results;

	const int _minRunningCount =
		(std::thread::hardware_concurrency() + 1) * 10;
	const int _maxRunningCount = _minRunningCount * 2;
	mutable std::unordered_map<CacheKey, Task> _tasks;
	mutable std::atomic<int> _tasksRunning{0};
	size_t _tasksRingBufSize = _maxRunningCount * 3;
	mutable std::vector<CacheKey> _tasksRingBuf;
	mutable size_t _tasksRBpos = 0;


	mutable PterosSystemLoader _systemLoader;

	std::unordered_map<std::string, EvalId> _evalNames; // main thread
	std::unordered_map<EvalId, EvalUPtr> _evals;	    // main thread
	std::vector<EvalUPtr> _removedEvals;
	EvalId _currentId;   // main thread
	EvalId _maxStubEval; // main thread

	std::thread::id tid_main;
	std::thread::id tid_worker;

public:
	// TODO: make theese const
	EvalId evaluatorPositionSimulation;
	EvalId evaluatorEvaluatorAvFile;
	EvalId evaluatorEvaluatorAvVolume;
	EvalId evaluatorFretEfficiency;
	EvalId evaluatorDistance;
	EvalId evaluatorChi2;
	EvalId evaluatorChi2r;
	EvalId evaluatorChi2Conribution;
	EvalId evaluatorWeightedResidual;
	EvalId evaluatorMinDistance;
	EvalId evaluatorAVOverlap;
	EvalId evaluatorTrasformationMatrix;
	EvalId evaluatorEulerAngle;
};

#endif // TASKSTORAGE_H
