#include "EvaluatorFretEfficiency.h"
#include "CalcResult.h"

EvaluatorFretEfficiency::
EvaluatorFretEfficiency(const TaskStorage &storage, const std::string &name):
	AbstractEvaluator(storage),_name(name)
{}

AbstractEvaluator::Task EvaluatorFretEfficiency::makeTask(const FrameDescriptor &frame) const noexcept
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

std::shared_ptr<AbstractCalcResult> EvaluatorFretEfficiency::calculate(const PositionSimulationResult &av1, const PositionSimulationResult &av2) const
{
	double eff=av1.meanFretEfficiency(av2,_R0);
	return std::make_shared<CalcResult<double>>(eff);
}
