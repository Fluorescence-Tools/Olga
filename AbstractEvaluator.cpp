#include "AbstractEvaluator.h"
#include "TaskStorage.h"


AbstractEvaluator::Task
AbstractEvaluator::getTask(const FrameDescriptor &desc,
			   const EvalId& evId,bool persistent) const
{
	return _storage.getTask(desc, evId,persistent);
}

AbstractEvaluator::PterosSysTask AbstractEvaluator::getSysTask(const FrameDescriptor &frame) const
{
	return _storage.getSysTask(frame);
}
