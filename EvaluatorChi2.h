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
	void updateDistances() {
		distances.clear();
		for(const auto& id:_distCalcs)
		{
			auto eval=static_cast<const EvaluatorDistance&>(_storage.eval(id));
			distances.push_back(eval._dist);
		}
	}

public:
	/*EvaluatorChi2(const TaskStorage& storage,
		       const std::vector<std::weak_ptr<EvaluatorDistance>> distCalcs);*/
	EvaluatorChi2(const TaskStorage& storage,const std::string& name):
		AbstractEvaluator(storage),_name(name){}
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
	virtual std::string className() const {
		return "χ²";
	}
	Setting setting(int row) const
	{
		if(row!=0) {
			return {"",""};
		}
		QList<EvalId> list;
		for(const auto& evid:_distCalcs) {
			if(_storage.isValid(evid)) {
				list.append(evid);
			}
		}
		if(list.isEmpty()) {
			list.append(_storage.evaluatorDistance);
		}
		return {"distances",QVariant::fromValue(list)};
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		if(row!=0) {
			return;
		}
		_distCalcs.clear();
		const QList<EvalId>& list=val.value<QList<EvalId>>();
		for(const EvalId& evId:list) {
			_distCalcs.push_back(evId);
		}
		updateDistances();
	}
	virtual void setName(const std::string& name)
	{
		_name=name;
	}
	virtual int settingsCount() const
	{
		return 1;
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
