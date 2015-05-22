#include "AbstractEvaluator.h"
#include "TaskStorage.h"


AbstractEvaluator::Task
AbstractEvaluator::getTask(const FrameDescriptor &desc,
			   const std::weak_ptr<AbstractEvaluator> eval,bool persistent) const
{
	auto evalShared=eval.lock();
	if(!evalShared) {
		std::cerr<<"Trying to get a task for the non-existing"
			   " evaluator. This should never happen."<<std::endl;
	}
	return _storage.getTask(desc, evalShared,persistent);
}

AbstractEvaluator::PterosSysTask AbstractEvaluator::getSysTask(const FrameDescriptor &frame) const
{
	return _storage.getSysTask(frame);
}
