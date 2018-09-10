#include "EvaluatorMinDistance.h"
#include "CalcResult.h"

EvaluatorMinDistance::EvaluatorMinDistance(const TaskStorage& storage,
					   const EvalId &av1,
					   const EvalId &av2,
					  Distance dist):
	AbstractEvaluator(storage),_av1(av1),_av2(av2),_dist(dist)
{

}

AbstractEvaluator::Task EvaluatorMinDistance::makeTask(const FrameDescriptor &frame) const noexcept
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

std::shared_ptr<AbstractCalcResult> EvaluatorMinDistance::calculate(const PositionSimulationResult &av1, const PositionSimulationResult &av2) const
{
	return std::make_shared<CalcResult<double>>(av1.minDistance(av2));
}
