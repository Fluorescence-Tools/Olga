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
public:
	TaskStorage();
	~TaskStorage();
	using EvalPtr=std::shared_ptr<AbstractEvaluator>;
	using CacheKey=std::pair<FrameDescriptor,EvalPtr>;
	using Task=async::shared_task<std::shared_ptr<AbstractCalcResult>>;
	using PterosSysTask=async::shared_task<pteros::System>;
	const Task& getTask(const FrameDescriptor &frame, EvalPtr eval,
			    bool persistent=true) const;
	const PterosSysTask& getSysTask(const FrameDescriptor &frame) const;
	std::string getString(const FrameDescriptor &frame, EvalPtr eval, int col,
			      bool persistent=true) const;
	int numTasks() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		return _cache.size();
	}
	int numSysTasks() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		return _sysCache.size();
	}
	int numTasksScheduled() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		return _tasksScheduled;
	}
	int numTasksFinished() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		return _tasksFinished;
	}
	int numTasksSubmited() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		return _tasksScheduled+_requestList.size();
	}
private:
	int unfinishedScheduledCount() const
	{
		return _tasksScheduled-_tasksFinished;
	}
	bool taskExists(const FrameDescriptor &frame,EvalPtr eval) const
	{
		auto it=_cache.find(CacheKey(frame,eval));
		if(it!=_cache.end()) {
			return true;
		}
		return false;
	}
	Task& makeTask(const FrameDescriptor &frame, EvalPtr eval,
		       bool persistent) const;
	void runRequests() const
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		if(unfinishedScheduledCount()>minRunningCount) {
			async::spawn([this]{
				runRequests();
			});
			return;
		}
		while(!_requestList.empty() && maxRunningCount>unfinishedScheduledCount())
		{
			auto req=_requestList.begin();
			if(!taskExists(std::get<0>(*req),std::get<1>(*req)))
			{
				makeTask(std::get<0>(*req),
					 std::get<1>(*req),std::get<2>(*req));
			}
			_requestList.erase(req);
		}
		if(!_requestList.empty()) {
			async::spawn([this]{
				runRequests();
			});
		}
	}
private:
	mutable std::recursive_mutex g_mutex;
	const int maxRunningCount=50,minRunningCount=maxRunningCount/2;
	mutable std::unordered_set<std::tuple<FrameDescriptor,EvalPtr,bool>> _requestList;
	mutable std::unordered_map<CacheKey,Task> _cache;
	const size_t ringBufSize=50;
	mutable std::vector<CacheKey> _ringBuf;
	mutable size_t ringBufIndex=0;

	mutable std::unordered_map<FrameDescriptor,PterosSysTask> _sysCache;
	const size_t sysRingBufSize=20;
	mutable std::vector<FrameDescriptor> _sysRingBuf;
	mutable size_t sysRingBufIndex=0;
	PterosSystemLoader _systemLoader;

	mutable int _tasksScheduled=0;
	mutable std::atomic<int> _tasksFinished{0};
	mutable std::atomic<bool> _runRequstsScheduled{false};
};

#endif // TASKSTORAGE_H
