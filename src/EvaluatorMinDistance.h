#ifndef EVALUATORMINDISTANCE_H
#define EVALUATORMINDISTANCE_H

#include "AbstractEvaluator.h"
#include "AV/Distance.h"

class EvaluatorMinDistance: public AbstractEvaluator
{
private:
	EvalId _av1, _av2;
	Distance _dist;
public:
	EvaluatorMinDistance(const TaskStorage& storage,
			     const EvalId& av1,
			     const EvalId& av2,
			     Distance dist);
	EvaluatorMinDistance(const TaskStorage& storage, const std::string& name):
		AbstractEvaluator(storage)
	{
		_dist.setName(name);
	}
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _dist.name();
	}
	virtual std::string className() const {
		return "Minimum distances";
	}
	virtual std::string columnName(int) const
	{
		return name();
	}
	virtual int columnCount() const
	{
		return 1;
	}
	virtual int settingsCount() const
	{
		return 5;
	}
	virtual Setting setting(int row) const override
	{
		switch(row)
		{
		case 0:
		{
			EvalId id=_storage.isValid(_av1)?_av1:_storage.evaluatorPositionSimulation;
			return {"position1_name",QVariant::fromValue(id)};
		}
		case 1:
		{
			EvalId id=_storage.isValid(_av2)?_av2:_storage.evaluatorPositionSimulation;
			return {"position2_name",QVariant::fromValue(id)};
		}
		case 2:
			return {"distance",_dist.distance()};
		case 3:
			return {"error_neg",_dist.errNeg()};
		case 4:
			return {"error_pos",_dist.errPos()};
		}
		return {"",""};
	}
	//std::string str;
	virtual void setSetting(int row, const QVariant& val)
	{
		switch(row)
		{
		case 0:
			_av1=val.value<EvalId>();
			//str=_storage.eval(_av1).name();
			//qDebug()<<"av1="<<static_cast<int>(_av1)<<", "<<str;
			_dist.setPosition1(_storage.eval(_av1).name());
			return;
		case 1:
			_av2=val.value<EvalId>();
			//str=_storage.eval(_av2).name();
			//qDebug()<<"av2="<<static_cast<int>(_av2)<<", "<<str;
			_dist.setPosition2(_storage.eval(_av2).name());
			return;
		case 2:
			_dist.setDistance(val.toDouble());
			return;
		case 3:
			_dist.setErrNeg(val.toDouble());
			return;
		case 4:
			_dist.setErrPos(val.toDouble());
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

#endif // EVALUATORMINDISTANCE_H
