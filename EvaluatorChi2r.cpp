#include "EvaluatorChi2r.h"
#include "CalcResult.h"

AbstractEvaluator::Task EvaluatorChi2r::makeTask(const FrameDescriptor &frame) const noexcept
{
	std::vector<Task> tasks;
	for(const auto& calc:_distCalcs)
	{
		tasks.push_back(getTask(frame,calc,true));
	}
	return async::when_all(tasks).then([this](std::vector<Task> tasks){
		double chi2=0.0;
		int i=0;
		for(const auto& task:tasks)
		{
			auto res=dynamic_cast<CalcResult<double>*>(task.get().get());
			double dist=res->get();
			double delta=(dist-distances[i].distance())/distances[i].err(dist);
			++i;
			chi2+=delta*delta;
		}
		auto result=std::make_shared<CalcResult<double>>(chi2/(tasks.size()-fitParamCount));
		return std::shared_ptr<AbstractCalcResult>(result);
	}).share();
}
