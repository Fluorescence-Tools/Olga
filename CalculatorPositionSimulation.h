#ifndef CALCULATORPOSITIONSIMULATION_H
#define CALCULATORPOSITIONSIMULATION_H

#include "AbstractCalculator.h"
#include "AV/Position.h"

class CalculatorPositionSimulation : public AbstractCalculator
{
private:
	const std::weak_ptr<Position> _position;
public:
	CalculatorPositionSimulation(const ResultCache& results, const std::weak_ptr<Position> position);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
	virtual std::string name(int i) const
	{
		if(auto position=_position.lock())
		{
			return std::string("AV ")+
					position->name()+"";
		}
		return "unknown(expired)";
	}
	//~CalculatorPositionSimulation();
};

#endif // CALCULATORPOSITIONSIMULATION_H
