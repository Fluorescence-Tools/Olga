#ifndef EVALUATORDISTANCE_H
#define EVALUATORDISTANCE_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include "AV/Distance.h"

class EvaluatorDistance : public AbstractEvaluator
{
	friend class EvaluatorChi2;
private:
	const std::weak_ptr<EvaluatorPositionSimulation> _av1, _av2;
	const std::weak_ptr<Distance> _dist;
public:
	EvaluatorDistance(const ResultCache& results,
			   const std::weak_ptr<EvaluatorPositionSimulation>& av1,
			   const std::weak_ptr<EvaluatorPositionSimulation>& av2,
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

#endif // EVALUATORDISTANCE_H
