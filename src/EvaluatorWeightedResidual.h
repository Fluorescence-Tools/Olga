#ifndef EVALUATORWEIGHTEDRESIDUAL_H
#define EVALUATORWEIGHTEDRESIDUAL_H
#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorWeightedResidual : public AbstractEvaluator
{
private:
	EvalId _distCalc;
	Distance _dist;
	std::string _name;

public:
	EvaluatorWeightedResidual(const TaskStorage &storage,
				  const std::string &name);
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string columnName(int) const
	{
		return name();
	}
	virtual int columnCount() const
	{
		return 1;
	}
	virtual std::string name() const
	{
		return _name;
	}
	virtual std::string className() const
	{
		return "w.res.";
	}
	Setting setting(int row) const
	{
		if (row != 0) {
			return {"", ""};
		}
		EvalId id = _storage.isValid(_distCalc)
				    ? _distCalc
				    : _storage.evaluatorDistance;
		return {"distance", QVariant::fromValue(id)};
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		if (row != 0) {
			return;
		}
		_distCalc = val.value<EvalId>();
		auto eval = static_cast<const EvaluatorDistance &>(
			_storage.eval(_distCalc));
		_dist = eval._dist;
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}
	virtual int settingsCount() const
	{
		return 1;
	}
};

#endif // EVALUATORWEIGHTEDRESIDUAL_H
