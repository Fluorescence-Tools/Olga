#ifndef EVALUATORCHI2_H
#define EVALUATORCHI2_H

#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2 : public AbstractEvaluator
{
private:
	const std::vector<std::weak_ptr<EvaluatorDistance>> _distCalcs;
	std::vector<Distance> distances;
public:
	EvaluatorChi2(const TaskStorage& storage,
		       const std::vector<std::weak_ptr<EvaluatorDistance>> distCalcs);

	virtual Task makeTask(const FrameDescriptor &frame) const;
	virtual std::string name(int) const
	{
		return "chi^2";
	}
};

#endif // EVALUATORCHI2_H
