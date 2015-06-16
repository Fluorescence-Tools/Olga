#include "EvaluatorChi2.h"
#include "EvaluatorDistance.h"
#include "CalcResult.h"
EvaluatorChi2::EvaluatorChi2(const TaskStorage &storage,
			       const std::vector<std::weak_ptr<EvaluatorDistance> > distCalcs):
	AbstractEvaluator(storage),_distCalcs(distCalcs)
{
	for(const auto& calc:_distCalcs)
	{
		std::shared_ptr<EvaluatorDistance> calcRef=calc.lock();
		if(!calcRef) {
			std::cerr<<"Error!"<<std::endl;
		}
		std::shared_ptr<Distance> dist=calcRef->_dist.lock();
		if(!dist) {
			std::cerr<<"Error!"<<std::endl;
		}
		distances.push_back(*dist);
	}
}

AbstractEvaluator::Task EvaluatorChi2::makeTask(const FrameDescriptor &frame) const
{
	std::vector<Task> tasks;
	for(const auto& calc:_distCalcs)
	{
		std::shared_ptr<EvaluatorDistance> calcRef=calc.lock();
		if(!calcRef) {
			std::cerr<<"Error!"<<std::endl;
		}
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
		auto result=std::make_shared<CalcResult<double>>(chi2);
		return std::shared_ptr<AbstractCalcResult>(result);
	}).share();

}
