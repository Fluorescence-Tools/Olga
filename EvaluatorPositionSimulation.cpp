#include "EvaluatorPositionSimulation.h"
#include "CalcResult.h"
std::shared_ptr<AbstractCalcResult> EvaluatorPositionSimulation::calculate(const pteros::System &system) const
{
	auto position = _position.lock();
	if( !position ) {
		std::cerr<<"Error! Position for AV calculation "
			   "does not exist anymore."<<std::endl;
		return std::shared_ptr<AbstractCalcResult>();
	}
	auto res=position->calculate(system);
	if(res.empty()) {
		std::cerr<<"simulation "+position->name()+" failed: empty AV\n";
	}
	//res.dumpShellXyz(position->name()+".xyz");
	return std::make_shared<CalcResult<PositionSimulationResult>>(std::move(res));
}

EvaluatorPositionSimulation::
EvaluatorPositionSimulation(const TaskStorage& storage,
			    const std::weak_ptr<Position> position):
	AbstractEvaluator(storage),_position(position)
{

}

AbstractEvaluator::Task EvaluatorPositionSimulation::makeTask(const FrameDescriptor &frame) const
{
	auto sysTask=getSysTask(frame);
	return sysTask.then([this](pteros::System system) {
		return calculate(system);
	}).share();
}
