#ifndef EVALUATORTRANSORMATIONMATRIX_H
#define EVALUATORTRANSORMATIONMATRIX_H

#include "AbstractEvaluator.h"
#include "AV/MolecularSystemDomain.h"

class EvaluatorTrasformationMatrix : public AbstractEvaluator
{
private:
	const std::weak_ptr<MolecularSystemDomain> _domain;
public:
	EvaluatorTrasformationMatrix(const TaskStorage& storage,
				     const std::weak_ptr<MolecularSystemDomain> domain);
	virtual Task
		makeTask(const FrameDescriptor &frame) const;
	virtual std::string name(int i=0) const
	{
		(void)i;
		if(auto domain=_domain.lock())
		{
			return std::string("")+
					domain->name.toStdString()+"";
		}
		return "unknown(expired)";
	}
private:
	std::shared_ptr<AbstractCalcResult> calculate(const pteros::System &system) const;
};

#endif // EVALUATORTRANSORMATIONMATRIX_H
