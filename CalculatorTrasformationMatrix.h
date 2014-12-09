#ifndef CALCULATORTRANSORMATIONMATRIX_H
#define CALCULATORTRANSORMATIONMATRIX_H

#include "AbstractCalculator.h"
#include "AV/MolecularSystemDomain.h"

class CalculatorTrasformationMatrix : public AbstractCalculator
{
private:
	const std::weak_ptr<MolecularSystemDomain> _domain;
public:
	CalculatorTrasformationMatrix(const std::weak_ptr<MolecularSystemDomain> domain);
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const pteros::System& system) const;
	virtual std::string name() const
	{
		if(auto domain=_domain.lock())
		{
			return std::string("TrasformationMatrix (")+
					domain->name.toStdString()+")";
		}
		return "unknown(expired)";
	}
};

#endif // CALCULATORTRANSORMATIONMATRIX_H
