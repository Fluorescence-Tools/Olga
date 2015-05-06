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
	const Task& getTask(const FrameDescriptor &frame, EvalPtr eval) const;
	const PterosSysTask& getSysTask(const FrameDescriptor &frame) const;
	std::string getString(const FrameDescriptor &frame, EvalPtr eval, int col);
private:
	Task& makeTask(const FrameDescriptor &frame, EvalPtr eval) const;

private:
	mutable std::unordered_map<CacheKey,Task> _cache;
	const size_t ringBufSize=50;
	mutable std::vector<CacheKey> _ringBuf;
	mutable size_t ringBufIndex=0;

	mutable std::unordered_map<FrameDescriptor,PterosSysTask> _sysCache;
	mutable std::vector<CacheKey> _sysRingBuf;
	mutable size_t sysRingBufIndex=0;
	PterosSystemLoader _systemLoader;
};

#endif // TASKSTORAGE_H
