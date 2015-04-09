#ifndef CALCULATORDISTANCE_H
#define CALCULATORDISTANCE_H

#include "AbstractCalculator.h"
#include "CalculatorPositionSimulation.h"
#include "AV/Distance.h"

class CalculatorDistance : public AbstractCalculator
{
	friend class CalculatorChi2;
private:
	const std::weak_ptr<CalculatorPositionSimulation> _av1, _av2;
	const std::weak_ptr<Distance> _dist;
public:
	CalculatorDistance(const ResultCache& results,
			   const std::weak_ptr<CalculatorPositionSimulation>& av1,
			   const std::weak_ptr<CalculatorPositionSimulation>& av2,
			   const std::weak_ptr<Distance>& dist);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
	virtual std::string name(int) const
	{
		auto av1=_av1.lock();
		auto av2=_av2.lock();
		auto dist=_dist.lock();
		if(av1 && av2 && dist) {
			return ""+dist->name()+"";
		}
		return "unknown(expired)";
	}
};

#endif // CALCULATORDISTANCE_H
