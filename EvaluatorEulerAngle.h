#ifndef EVALUATOREULERANGLE_H
#define EVALUATOREULERANGLE_H

#include "AbstractEvaluator.h"
#include "EvaluatorTrasformationMatrix.h"

class EvaluatorEulerAngle : public AbstractEvaluator
{
private:
	std::weak_ptr<const EvaluatorTrasformationMatrix> _refCalc, _bodyCalc;
	std::string _name;
public:
	EvaluatorEulerAngle(const TaskStorage& storage, const std::string& name):
		AbstractEvaluator(storage),_name(name)
	{	}
	virtual Task makeTask(const FrameDescriptor &frame) const;
	virtual std::string name() const
	{
		return _name;
	}
	virtual void setName(const std::string& name)
	{
		_name=name;
	}
	virtual std::string className() const {
		return "Euler angles";
	}
	virtual std::string columnName(int i) const
	{
		switch (i) {
		case 0:
			return _name+" α";
		case 1:
			return _name+" β";
		case 2:
			return _name+" γ";
		}
		return "";
	}
	virtual int columnCount() const
	{
		return 3;
	}
	virtual int settingsCount() const
	{
		return 2;
	}
	virtual Setting setting(int row) const override
	{
		switch(row)
		{
		case 0:
		{
			EvalPtr ref=_refCalc.lock();
			if(!ref) {
				ref=std::make_shared<EvaluatorTrasformationMatrix>(_storage,"unknown");
			}
			return {"reference_body",QVariant::fromValue(ref)};
		}
		case 1:
		{
			EvalPtr body=_bodyCalc.lock();
			if(!body) {
				body=std::make_shared<EvaluatorTrasformationMatrix>(_storage,"unknown");
			}
			return {"target_body",QVariant::fromValue(body)};
		}
		}
		return {"",""};
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		switch(row)
		{
		case 0:
			_refCalc=std::static_pointer_cast<const EvaluatorTrasformationMatrix>(val.value<std::shared_ptr<const AbstractEvaluator>>());
			return;
		case 1:
			_bodyCalc=std::static_pointer_cast<const EvaluatorTrasformationMatrix>(val.value<std::shared_ptr<const AbstractEvaluator>>());
			return;
		}
	}


private:
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const Eigen::Matrix4d& ref,
			  const Eigen::Matrix4d& body) const;
};

#endif // EVALUATOREULERANGLE_H
