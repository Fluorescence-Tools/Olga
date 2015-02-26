#include "CalculatorPositionSimulation.h"
#include "CalcResult.h"
CalculatorPositionSimulation::CalculatorPositionSimulation(const ResultCache& results, const std::weak_ptr<Position> position):
	AbstractCalculator(results),_position(position)
{

}
/*
CalculatorPositionSimulation::~CalculatorPositionSimulation()
{

}
*/


std::shared_ptr<AbstractCalcResult> CalculatorPositionSimulation::calculate(const FrameDescriptor &desc) const
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
