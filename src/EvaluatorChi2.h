#ifndef EVALUATORCHI2_H
#define EVALUATORCHI2_H

#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2 : public AbstractEvaluator
{
private:
	std::vector<EvalId> _distCalcs;
	std::vector<Distance> distances;
	std::string _name;
	int maxNanAlowed = 0;
	float nanPenalty = 0.0;
	void updateDistances()
	{
		distances.clear();
		for (const auto &id : _distCalcs) {
			try {
				auto eval =
					dynamic_cast<const EvaluatorDistance &>(
						_storage.eval(id));
				distances.push_back(eval._dist);
			} catch (std::bad_cast exp) {
				std::cerr<<"ERROR! In Chi2 '" + _name + "': Evaluator '"+_storage.eval(id).name() + "' type is not Distance, as it shoud be!\n";
			}
		}
	}

public:
	/*EvaluatorChi2(const TaskStorage& storage,
		       const std::vector<std::weak_ptr<EvaluatorDistance>>
	   distCalcs);*/
	EvaluatorChi2(const TaskStorage &storage, const std::string &name)
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
		return "χ²";
	}
	Setting setting(int row) const
	{
		switch (row) {
		case 0: {
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
		}
		case 1:
			return {"maximum_NaNs_allowed", maxNanAlowed};
		case 2:
			return {"penalty_NaN", nanPenalty};
		default:
			return {"", ""};
		}
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		switch (row) {
		case 0: {
			_distCalcs.clear();
			const QList<EvalId> &list = val.value<QList<EvalId>>();
			for (const EvalId &evId : list) {
				_distCalcs.push_back(evId);
			}
			updateDistances();
			break;
		}
		case 1:
			maxNanAlowed = val.toInt();
			break;
		case 2:
			nanPenalty = val.toFloat();
		default:
			return;
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
	/*virtual bool inline operator==(AbstractEvaluator& o) const
	{
		if(this==&o) {
			return true;
		}
		auto* new_o = dynamic_cast<const EvaluatorChi2*>(&o);
		if(new_o) {
			if(!(distances==new_o->distances)) {
				return false;
			}
			//_distCalcs != new_o->_distCalcs
			return true;
		}
		return false;
	}*/
};

#endif // EVALUATORCHI2_H
