#ifndef EVALUATORCHI2R_H
#define EVALUATORCHI2R_H
#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2r : public AbstractEvaluator
{
private:
	std::vector<EvalId> _distCalcs;
	std::vector<Distance> distances;
	std::string _name;
	void updateDistances()
	{
		distances.clear();
		for (const auto &id : _distCalcs) {
			auto eval = static_cast<const EvaluatorDistance &>(
				_storage.eval(id));
			distances.push_back(eval._dist);
		}
	}
	int fitParamCount = 0;
	bool ignoreNan = false;

public:
	EvaluatorChi2r(const TaskStorage &storage, const std::string &name)
	    : AbstractEvaluator(storage), _name(name)
	{
	}
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
		return "χᵣ²";
	}
	Setting setting(int row) const
	{
		if (row == 0) {
			QList<EvalId> list;
			for (const auto &evid : _distCalcs) {
				if (_storage.isValid(evid)) {
					list.append(evid);
				}
			}
			if (list.isEmpty()) {
				list.append(_storage.evaluatorDistance);
			}
			return {"distances", QVariant::fromValue(list)};
		} else if (row == 1) {
			return {"number of fit parameters", fitParamCount};
		} else if (row == 2) {
			return {"ignore_nan", ignoreNan};
		}
		return {"", ""};
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		if (row == 1) {
			fitParamCount = val.toInt();
			if (fitParamCount < 0) {
				fitParamCount = 0;
			}
		} else if (row == 0) {
			_distCalcs.clear();
			const QList<EvalId> &list = val.value<QList<EvalId>>();
			for (const EvalId &evId : list) {
				_distCalcs.push_back(evId);
			}
			updateDistances();
		} else if (row == 2) {
			ignoreNan = val.toBool();
		}
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}
	virtual int settingsCount() const
	{
		return 3;
	}
};

#endif // EVALUATORCHI2R_H
