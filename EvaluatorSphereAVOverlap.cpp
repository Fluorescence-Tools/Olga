#include "EvaluatorSphereAVOverlap.h"
#include "CalcResult.h"
#include "AV/PositionSimulationResult.h"

AbstractEvaluator::Task EvaluatorSphereAVOverlap::makeTask(const FrameDescriptor &frame) const noexcept
{
	PterosSysTask sysTask=getSysTask(frame);
	Task av=getTask(frame,_av1,false);
	using result_t=std::tuple<PterosSysTask,Task>;
	return async::when_all(sysTask,av).then([this](result_t result) {
		auto ptrAv=std::get<1>(result).get();
		auto resAv=dynamic_cast<CalcResult<PositionSimulationResult>*>(ptrAv.get());
		PositionSimulationResult av=resAv->get();
		pteros::System system=std::get<0>(result).get();
		return calculate(system,av);
	}).share();
}

std::shared_ptr<AbstractCalcResult> EvaluatorSphereAVOverlap::calculate(const pteros::System &system, const PositionSimulationResult &av) const
{
	pteros::Selection select;
	try
	{
		select.modify(system,_selectionString);
	}
	catch(const pteros::Pteros_error &err)
	{
		std::cerr<<err.what()<<std::endl;
	}

	float totalOverlap=0.0f;
	int selectedCount=select.size();
	for (int i=0; i<selectedCount; ++i) {
		Eigen::Vector3f ref=select.XYZ(i)*10.0f;
		totalOverlap+=av.overlap(ref,_overlapRadius);
	}

	return std::make_shared<CalcResult<float>>(totalOverlap);
}
