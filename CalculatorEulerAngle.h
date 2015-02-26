#ifndef CALCULATOREULERANGLE_H
#define CALCULATOREULERANGLE_H

#include "AbstractCalculator.h"
#include "CalculatorTrasformationMatrix.h"

class CalculatorEulerAngle : public AbstractCalculator
{
private:
	const std::weak_ptr<CalculatorTrasformationMatrix> _refCalc, _bodyCalc;
public:
	CalculatorEulerAngle(const ResultCache& results,
			     const std::weak_ptr<CalculatorTrasformationMatrix>& refCalc,
			     const std::weak_ptr<CalculatorTrasformationMatrix>& bodyCalc):
		AbstractCalculator(results),_refCalc(refCalc),_bodyCalc(bodyCalc)
	{
	}
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
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
};

#endif // CALCULATOREULERANGLE_H
