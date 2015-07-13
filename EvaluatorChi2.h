#ifndef EVALUATORCHI2_H
#define EVALUATORCHI2_H

#include "AbstractEvaluator.h"
#include "EvaluatorDistance.h"

class EvaluatorChi2 : public AbstractEvaluator
{
private:
	std::vector<std::weak_ptr<const EvaluatorDistance>> _distCalcs;
	std::vector<Distance> distances;
	std::string _name;
	void updateDistances() {
		distances.clear();
		for(const auto& calc:_distCalcs)
		{
			std::shared_ptr<const EvaluatorDistance> calcRef=calc.lock();
			if(!calcRef) {
				std::cerr<<"Error!"<<std::endl;
			}
			distances.push_back(calcRef->_dist);
		}
	}

public:
	/*EvaluatorChi2(const TaskStorage& storage,
		       const std::vector<std::weak_ptr<EvaluatorDistance>> distCalcs);*/
	EvaluatorChi2(const TaskStorage& storage,const std::string& name):
		AbstractEvaluator(storage),_name(name){}
	virtual Task makeTask(const FrameDescriptor &frame) const;
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
		QList<EvalPtr> list;
		for(const auto& wptr:_distCalcs) {
			const EvalPtr& eval=wptr.lock();
			if(eval) {
				list.append(eval);
			}
		}
		if(list.empty()) {
			list.append(std::make_shared<EvaluatorDistance>(_storage,"unknown"));
		}
		return {"distances",QVariant::fromValue(list)};
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		if(row!=0) {
			return;
		}
		_distCalcs.clear();
		const QList<EvalPtr>& list=val.value<QList<EvalPtr>>();
		for(const EvalPtr& eval:list) {
			const auto& ptr=std::static_pointer_cast<const EvaluatorDistance>(eval);
			_distCalcs.push_back(ptr);
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
