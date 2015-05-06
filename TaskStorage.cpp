#include "TaskStorage.h"
#include "AbstractEvaluator.h"

TaskStorage::TaskStorage():_ringBuf(ringBufSize)
{
}

TaskStorage::~TaskStorage()
{

}

const TaskStorage::Task &TaskStorage::getTask(const FrameDescriptor &frame, TaskStorage::EvalPtr eval) const
{
	auto it=_cache.find(CacheKey(frame,eval));
	if(it!=_cache.end()) {
		return it->second;
	}
	else {
		return makeTask(frame,eval);
	}
}

const TaskStorage::PterosSysTask &TaskStorage::getSysTask(const FrameDescriptor &frame) const
{
	auto it=_sysCache.find(frame);
	if(it!=_sysCache.end()) {
		return it->second;
	}
	else {
		auto pair=_sysCache.emplace(frame,_systemLoader.makeTask(frame));
		return pair.first->second;
	}
}

std::string TaskStorage::getString(const FrameDescriptor &frame, TaskStorage::EvalPtr eval, int col)
{
	auto task=getTask(frame,eval);
	if(task.ready()) {
		return task.get()->toString(col);
	}
	else {
		return "...";
	}
}

TaskStorage::Task &TaskStorage::
makeTask(const FrameDescriptor &frame, TaskStorage::EvalPtr eval) const
{
	auto pair=_cache.emplace(CacheKey(frame,eval),eval->makeTask(frame));
	return pair.first->second;
}

