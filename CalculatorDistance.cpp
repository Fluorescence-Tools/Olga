#include "CalculatorDistance.h"
#include "CalcResult.h"

CalculatorDistance::
CalculatorDistance(const AbstractCalculator::ResultCache &results,
		   const std::weak_ptr<CalculatorPositionSimulation> &av1,
		   const std::weak_ptr<CalculatorPositionSimulation> &av2, const std::weak_ptr<Distance> &dist):
	AbstractCalculator(results),_av1(av1),_av2(av2),_dist(dist)
{

}

std::shared_ptr<AbstractCalcResult> CalculatorDistance::calculate(const FrameDescriptor &desc) const
{
	auto av1=result(desc,_av1);
	auto av2=result(desc,_av2);
	auto dist=_dist.lock();
	if(!av1 || !av2 || !dist) {
		return std::shared_ptr<AbstractCalcResult>();
	}
	CalcResult<PositionSimulationResult> *pAv1, *pAv2;
	pAv1=dynamic_cast<CalcResult<PositionSimulationResult>*>(av1.get());
	pAv2=dynamic_cast<CalcResult<PositionSimulationResult>*>(av2.get());
	if(!pAv1 || !pAv2) {//wrong result type
		std::cerr<<"wrong result type, this should never happen."
			   "Unexpected behaviour in CalculatorDistance::calculate"+
			std::string(typeid(av1.get()).name())<<std::endl;
		return std::shared_ptr<AbstractCalcResult>();
	}
	double result=dist->modelDistance(pAv1->get(),pAv2->get());
	return std::make_shared<CalcResult<double>>(result);
}
