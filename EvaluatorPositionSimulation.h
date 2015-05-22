#ifndef EVALUATORPOSITIONSIMULATION_H
#define EVALUATORPOSITIONSIMULATION_H

#include "AbstractEvaluator.h"
#include "AV/Position.h"

class EvaluatorPositionSimulation : public AbstractEvaluator
{
private:
	const std::weak_ptr<Position> _position;
	std::shared_ptr<AbstractCalcResult> calculate(const pteros::System &system) const;
public:
	EvaluatorPositionSimulation(const TaskStorage& storage, const std::weak_ptr<Position> position);
	virtual Task
		makeTask(const FrameDescriptor &frame) const;
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
