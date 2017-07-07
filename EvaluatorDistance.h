#ifndef EVALUATORDISTANCE_H
#define EVALUATORDISTANCE_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include "AV/Distance.h"

class EvaluatorDistance : public AbstractEvaluator
{
	friend class EvaluatorChi2;
	friend class EvaluatorChi2Contribution;
	friend class EvaluatorChi2r;
	friend class EvaluatorWeightedResidual;
	//static EvaluatorPositionSimulation _avStub;
private:
	EvalId _av1=_storage.evaluatorPositionSimulation, _av2=_av1;
	Distance _dist;
public:
	EvaluatorDistance(const TaskStorage& storage,
			   const EvalId& av1,
			   const EvalId& av2,
			   Distance dist);
	EvaluatorDistance(const TaskStorage& storage, const std::string& name):
		AbstractEvaluator(storage)
	{
		_dist.setName(name);
	}
	EvaluatorDistance(const TaskStorage& storage, const QString &line,
			  const QString &type):
		AbstractEvaluator(storage)
	{
		_dist.setFromLegacy(line.toStdString(),type.toStdString());
		_av1=_storage.evalId(_dist.position1());
		_av2=_storage.evalId(_dist.position2());
	}
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
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
	virtual int columnCount() const
	{
		return 1;
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
			EvalId id=_storage.isValid(_av1)?_av1:_storage.evaluatorPositionSimulation;
			return {"position1_name",QVariant::fromValue(id)};
		}
		case 2:
		{
			EvalId id=_storage.isValid(_av2)?_av2:_storage.evaluatorPositionSimulation;
			return {"position2_name",QVariant::fromValue(id)};
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
	//std::string str;
	virtual void setSetting(int row, const QVariant& val)
	{
		switch(row)
		{
		case 0:
			_dist.setType(val.toString().toStdString());
			return;
		case 1:
		{
			auto tmpId=val.value<EvalId>();
			if (tmpId==EvalId(-1)) {
				std::cerr<<"Can not set position_1: "+name()+"\n"<<std::flush;
				return;
			}
			_av1=tmpId;
			//str=_storage.eval(_av1).name();
			//qDebug()<<"av1="<<static_cast<int>(_av1)<<", "<<str;
			_dist.setPosition1(_storage.eval(_av1).name());
			return;
		}
		case 2:
		{
			auto tmpId=val.value<EvalId>();
			if (tmpId==EvalId(-1)) {
				std::cerr<<"Can not set position_2: "+name()+"\n"<<std::flush;
				return;
			}
			_av2=tmpId;
			//str=_storage.eval(_av2).name();
			//qDebug()<<"av2="<<static_cast<int>(_av2)<<", "<<str;
			_dist.setPosition2(_storage.eval(_av2).name());
			return;
		}
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
	Distance distance() const
	{
		return _dist;
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const PositionSimulationResult& av1,
			  const PositionSimulationResult& av2) const;
};

#endif // EVALUATORDISTANCE_H
