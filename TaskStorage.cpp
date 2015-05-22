#include "TaskStorage.h"
#include "AbstractEvaluator.h"

TaskStorage::TaskStorage():_ringBuf(ringBufSize),_sysRingBuf(sysRingBufSize)
{
}

TaskStorage::~TaskStorage()
{

}

const TaskStorage::Task &TaskStorage::getTask(const FrameDescriptor &frame, TaskStorage::EvalPtr eval, bool persistent) const
{
	std::lock_guard<std::recursive_mutex> lock(g_mutex);
	auto it=_cache.find(CacheKey(frame,eval));
	if(it!=_cache.end()) {
		return it->second;
	}
	else {
		return makeTask(frame,eval,persistent);
	}
}

const TaskStorage::PterosSysTask &TaskStorage::getSysTask(const FrameDescriptor &frame) const
{
	auto it=_sysCache.find(frame);
	if(it!=_sysCache.end()) {
		return it->second;
	}
	else {
		auto oldKey=_sysRingBuf[sysRingBufIndex];
		_sysRingBuf[sysRingBufIndex]=frame;
		++sysRingBufIndex;
		sysRingBufIndex%=sysRingBufSize;
		_sysCache.erase(oldKey);
		auto pair=_sysCache.emplace(frame,_systemLoader.makeTask(frame));
		return pair.first->second;
	}
}

std::string TaskStorage::getString(const FrameDescriptor &frame,
				   TaskStorage::EvalPtr eval, int col,
				   bool persistent) const
{
	{
		std::lock_guard<std::recursive_mutex> lock(g_mutex);
		if(!taskExists(frame,eval) && unfinishedScheduledCount()>maxRunningCount) {
			_requestList.emplace(frame,eval,persistent);
			if(_requestList.size()==1)
			{
				async::spawn([this]{
					runRequests();
				});
			}
			return "...";
		}
	}
	auto task=getTask(frame,eval,persistent);
	if(task.ready()) {
		return task.get()->toString(col);
	}
	return "...";
}

TaskStorage::Task &TaskStorage::
makeTask(const FrameDescriptor &frame, TaskStorage::EvalPtr eval, bool persistent) const
{
	auto key=CacheKey(frame,eval);
	if(!persistent) {
		auto oldKey=_ringBuf[ringBufIndex];
		_ringBuf[ringBufIndex]=key;
		++ringBufIndex;
		ringBufIndex%=ringBufSize;
		_cache.erase(oldKey);
	}
	auto pair=_cache.emplace(std::move(key),eval->makeTask(frame));
	_tasksScheduled++;
	pair.first->second.then([this](Task t){
		(void)t;
		_tasksFinished++;
	});
	return pair.first->second;
}

