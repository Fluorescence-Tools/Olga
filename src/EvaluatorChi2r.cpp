#include "EvaluatorChi2r.h"
#include "CalcResult.h"
#include <cmath>
#include <limits>

AbstractEvaluator::Task
EvaluatorChi2r::makeTask(const FrameDescriptor &frame) const noexcept
{
	std::vector<Task> tasks;
	for (const auto &calc : _distCalcs) {
		tasks.push_back(getTask(frame, calc, true));
	}
	return async::when_all(tasks)
		.then([this](std::vector<Task> tasks) {
			double chi2 = 0.0;
			int i = 0;
			int nanCount = 0;
			for (const auto &task : tasks) {
				auto res = dynamic_cast<CalcResult<double> *>(
					task.get().get());
				double dist = res->get();
				double delta = (dist - distances[i].distance())
					       / distances[i].err(dist);
				++i;
				if (std::isnan(delta)) {
					delta = 0.0;
					++nanCount;
				}
				chi2 += delta * delta;
			}

			double chi2r;
			int N = tasks.size() - fitParamCount;
			if (ignoreNan) {
				N -= nanCount;
				if (N <= 0) {
					chi2 = std::numeric_limits<
						double>::quiet_NaN();
				}
			} else if (nanCount > 0) {
				chi2 = std::numeric_limits<double>::quiet_NaN();
			}
			chi2r = chi2 / N;
			auto result =
				std::make_shared<CalcResult<double>>(chi2r);
			return std::shared_ptr<AbstractCalcResult>(result);
		})
		.share();
}
