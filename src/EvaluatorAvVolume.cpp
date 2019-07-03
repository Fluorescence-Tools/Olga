#include "EvaluatorAvVolume.h"
#include "CalcResult.h"

AbstractEvaluator::Task
EvaluatorAvVolume::makeTask(const FrameDescriptor &frame) const noexcept
{
	Task av = getTask(frame, _av, false);
	using result_t = Task;
	return av
		.then([this](result_t result) {
			auto ptrAv = result.get();
			auto resAv = dynamic_cast<
				CalcResult<PositionSimulationResult> *>(
				ptrAv.get());
			PositionSimulationResult av = resAv->get();
			return calculate(av);
		})
		.share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorAvVolume::calculate(const PositionSimulationResult &av) const
{
	if (_onlyContact) {
		return std::make_shared<CalcResult<int>>(av.size()
							 - av.freeSize());
	}
	return std::make_shared<CalcResult<int>>(av.size());
}
