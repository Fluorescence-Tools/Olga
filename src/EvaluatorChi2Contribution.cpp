#include "EvaluatorChi2Contribution.h"
#include "EvaluatorDistance.h"
#include "CalcResult.h"

EvaluatorChi2Contribution::EvaluatorChi2Contribution(const TaskStorage &storage,
						     const std::string &name)
    : AbstractEvaluator(storage), _name(name)
{
}

AbstractEvaluator::Task
EvaluatorChi2Contribution::makeTask(const FrameDescriptor &frame) const noexcept
{
	auto t = getTask(frame, _distCalc, true);
	return t.then([this](Task task) {
			auto res = dynamic_cast<CalcResult<double> *>(
				task.get().get());
			double dist = res->get();
			double delta =
				(dist - _dist.distance()) / _dist.err(dist);
			double chi2c = delta * delta;
			auto result =
				std::make_shared<CalcResult<double>>(chi2c);
			return std::shared_ptr<AbstractCalcResult>(result);
		})
		.share();
}
