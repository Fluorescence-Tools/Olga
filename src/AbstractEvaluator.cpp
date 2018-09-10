#include "AbstractEvaluator.h"
#include "TaskStorage.h"

AbstractEvaluator::Task
AbstractEvaluator::getTask(const FrameDescriptor &desc,
			   const EvalId& evId,bool persistent) const
{
	try {
		return _storage.getTask(desc, evId,persistent);
	} catch (...) {
		std::cerr<<"ERROR! Could not create a task (exception): "
			   +desc.fullName()<<std::flush;
		return Task();
	}
}

AbstractEvaluator::PterosSysTask AbstractEvaluator::getSysTask(const FrameDescriptor &frame) const
{
	return _storage.getSysTask(frame);
}
