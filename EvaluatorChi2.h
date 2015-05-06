#ifndef EVALUATORCHI2_H
#define EVALUATORCHI2_H

#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2 : public AbstractEvaluator
{
private:
	const std::vector<std::weak_ptr<EvaluatorDistance>> _distCalcs;
public:
	EvaluatorChi2(const ResultCache& results,
		       const std::vector<std::weak_ptr<EvaluatorDistance>> distCalcs);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
	virtual std::string name(int) const
	{
		return "chi^2";
	}
};

#endif // EVALUATORCHI2_H
