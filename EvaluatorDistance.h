#ifndef EVALUATORDISTANCE_H
#define EVALUATORDISTANCE_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include "AV/Distance.h"

class EvaluatorDistance : public AbstractEvaluator
{
	friend class EvaluatorChi2;
private:
	std::weak_ptr<const EvaluatorPositionSimulation> _av1, _av2;
	Distance _dist;
public:
	EvaluatorDistance(const TaskStorage& storage,
			   const std::weak_ptr<EvaluatorPositionSimulation>& av1,
			   const std::weak_ptr<EvaluatorPositionSimulation>& av2,
			   Distance dist);
	EvaluatorDistance(const TaskStorage& storage):AbstractEvaluator(storage){}
	virtual Task makeTask(const FrameDescriptor &frame) const;
	virtual std::string name() const
	{
		return _dist.name();
	}
	virtual std::string className() const {
		return "Distances";
	}
	virtual std::string columnName(int) const
	{
		return name();
	}
	virtual int settingsCount() const
	{
		return 7;
	}
	virtual Setting setting(int row) const override
	{
		switch(row)
		{
		case 0:
			return {"distance_type",QString::fromStdString(_dist.type())};
		case 1:
		{
			EvalPtr av1=_av1.lock();
			if(!av1){
				av1=std::make_shared<EvaluatorPositionSimulation>(_storage,"unknown");
			}
			return {"position1_name",QVariant::fromValue(av1)};
		}
		case 2:
		{
			EvalPtr av2=_av2.lock();
			if(!av2){
				av2=std::make_shared<EvaluatorPositionSimulation>(_storage,"unknown");
			}
			return {"position2_name",QVariant::fromValue(av2)};
		}
		case 3:
			return {"distance",_dist.distance()};
		case 4:
			return {"error_neg",_dist.errNeg()};
		case 5:
			return {"error_pos",_dist.errPos()};
		case 6:
			return {"Forster_radius",_dist.R0()};
		}
		return {"",""};
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		switch(row)
		{
		case 0:
			_dist.setType(val.toString().toStdString());
			return;
		case 1:
			_av1=std::static_pointer_cast<const EvaluatorPositionSimulation>(val.value<std::shared_ptr<const AbstractEvaluator>>());
			_dist.setPosition1(_av1.lock()->name());
			return;
		case 2:
			_av2=std::static_pointer_cast<const EvaluatorPositionSimulation>(val.value<std::shared_ptr<const AbstractEvaluator>>());
			_dist.setPosition2(_av2.lock()->name());
			return;
		case 3:
			_dist.setDistance(val.toDouble());
			return;
		case 4:
			_dist.setErrNeg(val.toDouble());
			return;
		case 5:
			_dist.setErrPos(val.toDouble());
			return;
		case 6:
			_dist.setR0(val.toDouble());
			return;
		}
	}
	virtual void setName(const std::string& name)
	{
		_dist.setName(name);
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const PositionSimulationResult& av1,
			  const PositionSimulationResult& av2) const;
};

#endif // EVALUATORDISTANCE_H
