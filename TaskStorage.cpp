#include "TaskStorage.h"
#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"//TODO:remove

TaskStorage::TaskStorage():_tasksRingBuf(_tasksRingBufSize),_sysRingBuf(_sysRingBufSize)
{
}

TaskStorage::~TaskStorage()
{
}

//must only run in worker thread
const TaskStorage::Task &
TaskStorage::getTask(const FrameDescriptor &frame, const EvalPtr &eval,
		     bool persistent) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());
	auto key=CacheKey(frame,eval);

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
	Task& task=_tasks.emplace(key,key.second->makeTask(key.first)).first->second;
	pushTask(key);
	_tasksRunning++;

	task.then([this,key,persistent](Task tres){
		if(persistent) {
			_results.insert(key,tres.get());
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

//must only run in the main thread;
std::string TaskStorage::getString(const FrameDescriptor &frame,
				   const EvalPtr &eval, int col,
				   bool persistent) const
{
	static auto tid=std::this_thread::get_id();
	assert(tid==std::this_thread::get_id());
	auto key=CacheKey(frame,eval);
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
