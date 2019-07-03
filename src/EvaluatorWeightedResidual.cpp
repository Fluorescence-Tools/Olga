#include "EvaluatorWeightedResidual.h"
#include "EvaluatorDistance.h"
#include "CalcResult.h"

EvaluatorWeightedResidual::EvaluatorWeightedResidual(const TaskStorage &storage,
						     const std::string &name)
    : AbstractEvaluator(storage), _name(name)
{
}

AbstractEvaluator::Task
EvaluatorWeightedResidual::makeTask(const FrameDescriptor &frame) const noexcept
{
	auto t = getTask(frame, _distCalc, true);
	return t.then([this](Task task) {
			auto res = dynamic_cast<CalcResult<double> *>(
				task.get().get());
			double dist = res->get();
			double wres =
				(dist - _dist.distance()) / _dist.err(dist);
			auto result =
				std::make_shared<CalcResult<double>>(wres);
			return std::shared_ptr<AbstractCalcResult>(result);
		})
		.share();
}
