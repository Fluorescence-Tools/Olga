#ifndef TASKSTORAGE_H
#define TASKSTORAGE_H
#include "AbstractCalcResult.h"
//#include "AbstractEvaluator.h"
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

#include <readerwriterqueue/readerwriterqueue.h>

#include <libcuckoo/cuckoohash_map.hh>
template<typename K, typename V> using CuckooMap=cuckoohash_map<K, V, std::hash<K>>;
/*
inline int parseLine(char* line){
    int i = strlen(line);
    while (*line < '0' || *line > '9') line++;
    line[i-3] = '\0';
    i = atoi(line);
    return i;
}


inline size_t getMemUsed(){ //Note: this value is in bytes!
    FILE* file = fopen("/proc/self/status", "r");
    size_t result = -1;
    char line[128];


    while (fgets(line, 128, file) != NULL){
	if (strncmp(line, "VmSize:", 7) == 0){
	    result = parseLine(line);
	    break;
	}
    }
    fclose(file);
    return result;
}
inline void printMemUse()
{
    std::cout<<"memUsed: "+std::to_string((double)getMemUsed()/1024)+"MB\n";
    std::cout.flush();
}
*/
class AbstractEvaluator;
/*
template<class Tag, class Base = int> struct def_enum
{
    enum class type : Base { }; // Base задаёт ёмкость типа; можно сделать через пару минимум-максимум
};
using EvalIdBase=uint_fast16_t;
using EvalId=def_enum<EvalPtr,EvalIdBase>::type;
*/
using EvalPtr=std::shared_ptr<const AbstractEvaluator>;
using CacheKey=std::pair<FrameDescriptor,EvalPtr>;
namespace std {
//shared_ptr
/*template <>
struct hash<pair<FrameDescriptor,shared_ptr<AbstractEvaluator>>>
{
	size_t operator()(const pair<FrameDescriptor,shared_ptr<AbstractEvaluator>>& k) const
	{
		size_t seed=hash<shared_ptr<AbstractEvaluator>>()(k.second);
		hash_combine(seed,k.first);
		return seed;
	}
};
template <>
struct hash<tuple<FrameDescriptor,shared_ptr<AbstractEvaluator>,bool>>
{
	size_t operator()(const tuple<FrameDescriptor,shared_ptr<AbstractEvaluator>,bool>& k) const
	{
		size_t seed=hash<FrameDescriptor>()(get<0>(k));
		hash_combine(seed,get<1>(k));
		hash_combine(seed,get<2>(k));
		return seed;
	}
};*/
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
	using EvalPtr=::EvalPtr;
	using CacheKey=::CacheKey;
	using Result=std::shared_ptr<AbstractCalcResult>;
	using Task=async::shared_task<Result>;
	using PterosSysTask=async::shared_task<pteros::System>;

	std::string getString(const FrameDescriptor &frame, int calcNum, int col,
			      bool persistent=true) const;
	const PterosSysTask& getSysTask(const FrameDescriptor &frame) const;
	std::string getColumnName(int calcNum, int col) const;
	int getColumnCount(int calcNum) const;
	const AbstractEvaluator& eval(int i) const
	{
		return *_evals.at(i);
	}
	int evalCount() const
	{
		return _evals.size();
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
	void evaluate(const FrameDescriptor& frame, const std::vector<int>& evIds) const {
		for(int evId:evIds) {
			getString(frame,_evals[evId],0);
		}
	}
Q_SIGNALS:
	void evaluatorAdded(int evalNum);
	void evaluatorIsGoingToBeRemoved(int evalNum);
private:
	std::string getString(const FrameDescriptor &frame, const EvalPtr& eval, int col,
			      bool persistent=true) const;
	void addEvaluator(EvalPtr eval)
	{
		_evals.push_back(eval);
		Q_EMIT evaluatorAdded(evalCount()-1);
	}
	void removeEvaluator(int i) {
		Q_EMIT evaluatorIsGoingToBeRemoved(i);
		_evals.erase(_evals.begin()+i);
	}
	EvalPtr evalPtr(int i) const
	{
		return _evals.at(i);
	}
	/*std::string getString(const FrameDescriptor &frame, EvalPtr eval, int col,
			      bool persistent=true) const;*/
	/*const Task& getTask(const FrameDescriptor &frame, std::shared_ptr<AbstractEvaluator> eval,
			    bool persistent) const;*/
	const Task& getTask(const FrameDescriptor &frame, const EvalPtr& eval,
				    bool persistent) const;
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
				getTask(key.first,key.second,true);
			} else {
				_requests.reserve(0);
				return;
			}
		}
	}
private:
	mutable CuckooMap<CacheKey,bool> _requests;
	using RWQueue=moodycamel::ReaderWriterQueue<CacheKey>;
	mutable RWQueue _requestQueue;

	mutable CuckooMap<CacheKey,Result> _results;

	const int _maxRunningCount=50,_minRunningCount=_maxRunningCount/2;
	mutable std::unordered_map<CacheKey,Task> _tasks;
	mutable std::atomic<int> _tasksRunning{0};
	const size_t _tasksRingBufSize=_maxRunningCount*3;
	mutable std::vector<CacheKey> _tasksRingBuf;
	mutable size_t _tasksRBpos=0;
	mutable async::threadpool_scheduler workerPool{1};

	mutable std::unordered_map<FrameDescriptor,PterosSysTask> _sysCache;
	const size_t _sysRingBufSize=20;
	mutable std::vector<FrameDescriptor> _sysRingBuf;
	mutable size_t sysRingBufIndex=0;
	PterosSystemLoader _systemLoader;

	std::vector<EvalPtr> _evals;
};

#endif // TASKSTORAGE_H
