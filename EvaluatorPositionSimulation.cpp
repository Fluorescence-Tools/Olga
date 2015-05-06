#include "EvaluatorPositionSimulation.h"
#include "CalcResult.h"
EvaluatorPositionSimulation::EvaluatorPositionSimulation(const ResultCache& results, const std::weak_ptr<Position> position):
	AbstractEvaluator(results),_position(position)
{

}
/*
EvaluatorPositionSimulation::~EvaluatorPositionSimulation()
{

}
*/


std::shared_ptr<AbstractCalcResult> EvaluatorPositionSimulation::calculate(const FrameDescriptor &desc) const
{
	auto system=getSystem(desc);
	auto position = _position.lock();
	if( !position ) {
		std::cerr<<"Error! Position for AV calculation "
			   "does not exist anymore."<<std::endl;
		return std::shared_ptr<AbstractCalcResult>();
	}
	return std::make_shared<CalcResult<PositionSimulationResult>>(position->calculate(system));
}
