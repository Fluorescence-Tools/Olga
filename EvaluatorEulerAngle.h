#ifndef EVALUATOREULERANGLE_H
#define EVALUATOREULERANGLE_H

#include "AbstractEvaluator.h"
#include "EvaluatorTrasformationMatrix.h"

class EvaluatorEulerAngle : public AbstractEvaluator
{
private:
	const std::weak_ptr<EvaluatorTrasformationMatrix> _refCalc, _bodyCalc;
public:
	EvaluatorEulerAngle(const TaskStorage& storage,
			    const QVariantMap& props,
			    const std::weak_ptr<EvaluatorTrasformationMatrix>& refCalc,
			    const std::weak_ptr<EvaluatorTrasformationMatrix>& bodyCalc):
		AbstractEvaluator(storage),_refCalc(refCalc),_bodyCalc(bodyCalc)
	{	}
	virtual Task makeTask(const FrameDescriptor &frame) const;
	virtual std::string name(int i) const
	{
		auto ref=_refCalc.lock();
		auto body=_bodyCalc.lock();
		if(ref && body) {
			return "e"+std::to_string(i)+std::string(" (")+ref->name()+", "+
					body->name()+")";
		}
		return "unknown(expired)";
	}
	virtual std::string className() const {
		return "Euler angles";
	}
private:
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const Eigen::Matrix4d& ref,
			  const Eigen::Matrix4d& body) const;
};

#endif // EVALUATOREULERANGLE_H
