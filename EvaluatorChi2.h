#ifndef EVALUATORCHI2_H
#define EVALUATORCHI2_H

#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2 : public AbstractEvaluator
{
private:
	const std::vector<std::weak_ptr<EvaluatorDistance>> _distCalcs;
	std::vector<Distance> distances;
	std::string _name;
public:
	EvaluatorChi2(const TaskStorage& storage,
		       const std::vector<std::weak_ptr<EvaluatorDistance>> distCalcs);
	EvaluatorChi2(const TaskStorage& storage,const std::string& name):
		AbstractEvaluator(storage),_name(name){}
	virtual Task makeTask(const FrameDescriptor &frame) const;
	virtual std::string columnName(int) const
	{
		return name();
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
		//TODO:implement;
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		//TODO:implement;
	}
	virtual void setName(const std::string& name)
	{
		_name=name;
	}
	virtual int settingsCount() const
	{
		return distances.size();
	}
	virtual bool inline operator==(AbstractEvaluator& o) const
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
	}
};

#endif // EVALUATORCHI2_H
