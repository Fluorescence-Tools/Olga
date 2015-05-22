#include "EvaluatorDistance.h"
#include "CalcResult.h"

EvaluatorDistance::
EvaluatorDistance(const TaskStorage& storage,
		   const std::weak_ptr<EvaluatorPositionSimulation> &av1,
		   const std::weak_ptr<EvaluatorPositionSimulation> &av2, const std::weak_ptr<Distance> &dist):
	AbstractEvaluator(storage),_av1(av1),_av2(av2),_dist(dist)
{

}

AbstractEvaluator::Task EvaluatorDistance::makeTask(const FrameDescriptor &frame) const
{
	Task av1=getTask(frame,_av1,false);
	Task av2=getTask(frame,_av2,false);
	using result_t=std::tuple<Task,Task>;
	return async::when_all(av1,av2).then(
				[this](result_t result){
		auto ptrAv1=std::get<0>(result).get();
		auto ptrAv2=std::get<1>(result).get();
		auto resAv1=dynamic_cast<CalcResult<PositionSimulationResult>*>(ptrAv1.get());
		auto resAv2=dynamic_cast<CalcResult<PositionSimulationResult>*>(ptrAv2.get());
		PositionSimulationResult av1=resAv1->get();
		PositionSimulationResult av2=resAv2->get();
		return calculate(av1, av2);
	}).share();
}

std::shared_ptr<AbstractCalcResult> EvaluatorDistance::calculate(const PositionSimulationResult &av1, const PositionSimulationResult &av2) const
{
	auto dist=_dist.lock();
	if(!dist) {
		return std::shared_ptr<AbstractCalcResult>();
	}
	double result=dist->modelDistance(av1,av2);
	return std::make_shared<CalcResult<double>>(result);
}
