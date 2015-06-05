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

namespace std {
template <>
struct hash<std::pair<FrameDescriptor,shared_ptr<AbstractEvaluator>>>
{
	std::size_t operator()(const std::pair<FrameDescriptor,shared_ptr<AbstractEvaluator>>& k) const
	{
		size_t seed=hash<shared_ptr<AbstractEvaluator>>()(k.second);
		hash_combine(seed,k.first);
		return seed;
	}
};
template <>
struct hash<std::tuple<FrameDescriptor,shared_ptr<AbstractEvaluator>,bool>>
{
	std::size_t operator()(const std::tuple<FrameDescriptor,shared_ptr<AbstractEvaluator>,bool>& k) const
	{
		size_t seed=hash<FrameDescriptor>()(get<0>(k));
		hash_combine(seed,get<1>(k));
		hash_combine(seed,get<2>(k));
		return seed;
	}
};

}

class TaskStorage
{
	friend class AbstractEvaluator;
public:
	TaskStorage();
	~TaskStorage();
	using EvalPtr=std::shared_ptr<AbstractEvaluator>;
	using CacheKey=std::pair<FrameDescriptor,EvalPtr>;
	using Result=std::shared_ptr<AbstractCalcResult>;
	using Task=async::shared_task<Result>;
	using PterosSysTask=async::shared_task<pteros::System>;
	const PterosSysTask& getSysTask(const FrameDescriptor &frame) const;
	std::string getString(const FrameDescriptor &frame, EvalPtr eval, int col,
			      bool persistent=true) const;
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
private:
	const Task& getTask(const FrameDescriptor &frame, EvalPtr eval,
			    bool persistent) const;
	const Task& makeTask(const CacheKey &key,
			    bool persistent) const;
	inline void pushTask(const CacheKey &key) const
	{
		auto &oldK=_tasksRingBuf[_tasksRBpos];
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
				makeTask(key,true);
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
	const size_t _sysRingBufSize=10;
	mutable std::vector<FrameDescriptor> _sysRingBuf;
	mutable size_t sysRingBufIndex=0;
	PterosSystemLoader _systemLoader;
};

#endif // TASKSTORAGE_H
