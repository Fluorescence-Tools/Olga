#ifndef EVALUATORPOSITIONSIMULATION_H
#define EVALUATORPOSITIONSIMULATION_H

#include "AbstractEvaluator.h"
#include "AV/Position.h"

class EvaluatorPositionSimulation : public AbstractEvaluator
{
private:
	const std::weak_ptr<Position> _position;
public:
	EvaluatorPositionSimulation(const ResultCache& results, const std::weak_ptr<Position> position);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
	virtual std::string name(int ) const
	{
		if(auto position=_position.lock())
		{
			return std::string("AV ")+
					position->name()+"";
		}
		return "unknown(expired)";
	}
	//~EvaluatorPositionSimulation();
};

#endif // EVALUATORPOSITIONSIMULATION_H
