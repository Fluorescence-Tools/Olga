#ifndef EVALUATORFRETEFFICIENCY_H
#define EVALUATORFRETEFFICIENCY_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"

class EvaluatorFretEfficiency : public AbstractEvaluator
{
private:
	EvalId _av1, _av2;
	double _R0 = 52.0;
	std::string _name;

public:
	EvaluatorFretEfficiency(const TaskStorage &storage,
				const std::string &name);
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _name;
	}
	virtual std::string className() const
	{
		return "Mean FRET Efficiencies";
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
		case 0: {
			EvalId id =
				_storage.isValid(_av1)
					? _av1
					: _storage.evaluatorPositionSimulation;
			return {"position1_name", QVariant::fromValue(id)};
		}
		case 1: {
			EvalId id =
				_storage.isValid(_av2)
					? _av2
					: _storage.evaluatorPositionSimulation;
			return {"position2_name", QVariant::fromValue(id)};
		}
		case 2:
			return {"Forster_radius", _R0};
		}
		return {"", ""};
	}
	// std::string str;
	virtual void setSetting(int row, const QVariant &val)
	{
		switch (row) {
		case 0: {
			auto tmpId = val.value<EvalId>();
			if (tmpId == EvalId(-1)) {
				std::cerr << "Can not set position_1\n"
					  << std::flush;
				return;
			}
			_av1 = tmpId;
			// str=_storage.eval(_av1).name();
			// qDebug()<<"av1="<<static_cast<int>(_av1)<<", "<<str;
			return;
		}
		case 1: {
			auto tmpId = val.value<EvalId>();
			if (tmpId == EvalId(-1)) {
				std::cerr << "Can not set position_2\n"
					  << std::flush;
				return;
			}
			_av2 = tmpId;
			// str=_storage.eval(_av2).name();
			// qDebug()<<"av2="<<static_cast<int>(_av2)<<", "<<str;
			return;
		}
		case 2:
			_R0 = val.toDouble();
			return;
		}
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
	calculate(const PositionSimulationResult &av1,
		  const PositionSimulationResult &av2) const;
};

#endif // EVALUATORFRETEFFICIENCY_H
