#include "CalculatorChi2.h"
#include "CalculatorDistance.h"
#include "CalcResult.h"
CalculatorChi2::CalculatorChi2(const AbstractCalculator::ResultCache &results,
			       const std::vector<std::weak_ptr<CalculatorDistance> > distCalcs):
	AbstractCalculator(results),_distCalcs(distCalcs)
{

}

std::shared_ptr<AbstractCalcResult> CalculatorChi2::calculate(const FrameDescriptor &desc) const
{
	double chi2=0.0;
	for(const auto& calc:_distCalcs)
	{
		std::shared_ptr<CalculatorDistance> calcRef=calc.lock();
		if(!calcRef) {
			return std::shared_ptr<AbstractCalcResult>();
		}
		std::shared_ptr<Distance> dist=calcRef->_dist.lock();
		if(!dist) {
			return std::shared_ptr<AbstractCalcResult>();
		}
		auto modelDist=result(desc,calc);
		if(!modelDist) {
			return std::shared_ptr<AbstractCalcResult>();
		}
		CalcResult<double> *model=dynamic_cast<CalcResult<double>*>(modelDist.get());
		if(!model) {
			std::cerr<<"wrong result type, this should never happen."
				   "Unexpected behaviour in CalculatorChi2::calculate"
				<<std::endl;
			return std::shared_ptr<AbstractCalcResult>();
		}
		double delta=(model->get()-dist->distance())/dist->err(model->get());
		chi2+=delta*delta;
	}
	return std::make_shared<CalcResult<double>>(chi2);
}
