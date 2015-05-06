#ifndef ABSTRACTEVALUATOR_H
#define ABSTRACTEVALUATOR_H
//#include "TaskStorage.h"
#include "AbstractCalcResult.h"
#include "FrameDescriptor.h"

#include <pteros/pteros.h>

#include <async++.h>

#include <vector>
#include <memory>

class TaskStorage;
class AbstractEvaluator
{
private:
	const TaskStorage& _storage;
public:
	virtual ~AbstractEvaluator() {}
	AbstractEvaluator(const TaskStorage& storage):_storage(storage) {}
	using Task=async::shared_task<std::shared_ptr<AbstractCalcResult>>;
	using PterosSysTask=async::shared_task<pteros::System>;
	virtual Task makeTask(const FrameDescriptor &frame) const=0;
	virtual std::string name(int i) const=0;
protected:
	Task getTask(const FrameDescriptor &desc, const std::weak_ptr<AbstractEvaluator> eval) const;
	PterosSysTask getSysTask(const FrameDescriptor &frame) const;
};
Q_DECLARE_METATYPE(std::shared_ptr<AbstractEvaluator>)
bool inline operator==(const AbstractEvaluator& lhs, const AbstractEvaluator& rhs)
{
    return &lhs == &rhs;
}
#endif // ABSTRACTEVALUATOR_H
