#ifndef CALCULATORTRANSORMATIONMATRIX_H
#define CALCULATORTRANSORMATIONMATRIX_H

#include "AbstractCalculator.h"
#include "AV/MolecularSystemDomain.h"

class CalculatorTrasformationMatrix : public AbstractCalculator
{
private:
	const std::weak_ptr<MolecularSystemDomain> _domain;
public:
	CalculatorTrasformationMatrix(const ResultCache& results, const std::weak_ptr<MolecularSystemDomain> domain);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const FrameDescriptor& desc) const;
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
};

#endif // CALCULATORTRANSORMATIONMATRIX_H
