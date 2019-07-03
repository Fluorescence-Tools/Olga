#ifndef EVALUATORSPHEREAVOVERLAP_H
#define EVALUATORSPHEREAVOVERLAP_H

#include "AbstractEvaluator.h"

class PositionSimulationResult;

class EvaluatorSphereAVOverlap : public AbstractEvaluator
{
private:
	EvalId _av1;
	std::string _name;
	std::string _selectionString;
	float _overlapRadius = 0.0f;

public:
	EvaluatorSphereAVOverlap(const TaskStorage &storage,
				 const std::string &name)
	    : AbstractEvaluator(storage), _name(name)
	{
	}
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _name;
	}
	virtual std::string className() const
	{
		return "AV-sphere overlap";
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
		return 3;
	}
	virtual Setting setting(int row) const override
	{
		switch (row) {
		case 0:
			return {"selection_mask",
				QString::fromStdString(_selectionString)};
		case 1:
			return {"overlap_radius", _overlapRadius};
		case 2: {
			EvalId id =
				_storage.isValid(_av1)
					? _av1
					: _storage.evaluatorPositionSimulation;
			return {"position_name", QVariant::fromValue(id)};
		}
		}
		return Setting();
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		switch (row) {
		case 0:
			_selectionString = val.toString().toStdString();
			return;
		case 1:
			_overlapRadius = val.toFloat();
			return;
		case 2:
			_av1 = val.value<EvalId>();
			return;
		}
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
	calculate(const pteros::System &system,
		  const PositionSimulationResult &av) const;
};

#endif // EVALUATORSPHEREAVOVERLAP_H
