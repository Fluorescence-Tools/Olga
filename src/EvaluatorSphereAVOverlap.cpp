#include "EvaluatorSphereAVOverlap.h"
#include "CalcResult.h"
#include "AV/PositionSimulationResult.h"

AbstractEvaluator::Task
EvaluatorSphereAVOverlap::makeTask(const FrameDescriptor &frame) const noexcept
{
	PterosSysTask sysTask = getSysTask(frame);
	Task av = getTask(frame, _av1, false);
	using result_t = std::tuple<PterosSysTask, Task>;
	return async::when_all(sysTask, av)
	        .then([this](result_t result) {
		        auto ptrAv = std::get<1>(result).get();
			auto resAv = dynamic_cast<
			        CalcResult<PositionSimulationResult> *>(
			        ptrAv.get());
			PositionSimulationResult av = resAv->get();
			pteros::System system = std::get<0>(result).get();
			return calculate(system, av);
	        })
	        .share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorSphereAVOverlap::calculate(const pteros::System &system,
                                    const PositionSimulationResult &av) const
{
	pteros::Selection select;
	try {
		select.modify(system, _selectionString);
	} catch (const pteros::Pteros_error &err) {
		std::cerr << err.what() << std::endl;
	}

	int selectedCount = select.size();
	std::vector<Eigen::Vector3f> refs;
	for (int i = 0; i < selectedCount; ++i) {
		refs.push_back(select.xyz(i) * 10.0f);
	}

	return std::make_shared<CalcResult<float>>(
	        av.overlap(refs, _overlapRadius));
}
