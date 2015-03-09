#ifndef CALCULATORCHI2_H
#define CALCULATORCHI2_H

#include "AbstractCalculator.h"
#include "CalculatorDistance.h"

class CalculatorChi2 : public AbstractCalculator
{
private:
	const std::vector<std::weak_ptr<CalculatorDistance>> _distCalcs;
public:
	CalculatorChi2(const ResultCache& results,
		       const std::vector<std::weak_ptr<CalculatorDistance>> distCalcs);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
	virtual std::string name(int i) const
	{
		return "chi^2";
	}
};

#endif // CALCULATORCHI2_H
