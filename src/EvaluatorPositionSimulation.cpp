#include "EvaluatorPositionSimulation.h"
#include "CalcResult.h"

std::shared_ptr<AbstractCalcResult>
EvaluatorPositionSimulation::calculate(const pteros::System &system,
				       const FrameDescriptor &frame) const
{
	PositionSimulationResult res = _position.calculate(system);
	if (res.empty()) {
		std::cout << "Empty AV: " + _position.name() + ", "
				     + frame.fullName() + "\n";
	}
	/*std::string fname=_position.name();
	std::replace(fname.begin(),fname.end(),'/','_');
	res.dumpShellXyz(frame.trajFileName()+"_"+fname+".xyz");
	std::cout<<"dumping!
	"+frame.trajFileName()+"_"+fname+".xyz\n"<<std::flush;*/
	return std::make_shared<CalcResult<PositionSimulationResult>>(
		std::move(res));
}

AbstractEvaluator::Task EvaluatorPositionSimulation::makeTask(
	const FrameDescriptor &frame) const noexcept
{
	auto sysTask = getSysTask(frame);
	return sysTask
		.then([this, frame](pteros::System system) {
			return calculate(system, frame);
		})
		.share();
}
