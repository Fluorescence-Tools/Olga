#include <iostream>

#include <Eigen/Core>
#include <Eigen/Dense>

#include "CalculatorTrasformationMatrix.h"
#include "CalcResult.h"


CalculatorTrasformationMatrix::
CalculatorTrasformationMatrix(const std::weak_ptr<MolecularSystemDomain> domain) :
	_domain(domain)
{
}

std::shared_ptr<AbstractCalcResult> CalculatorTrasformationMatrix::calculate(const pteros::System &system) const
{
	Eigen::Matrix4d matrix;
	auto domain = _domain.lock();
	if( !domain )
	{
		std::cerr<<"Error! Domain for trasformation matrix calculation "
			   "does not exist anymore."<<std::endl;
	}

	size_t nPoints=std::min(domain->COMselections.size(),
				domain->COMpositionLocalCS.size());
	using Eigen::Dynamic;
	Eigen::Matrix<double,3,Dynamic> positionGlobalCS(3,nPoints);
	Eigen::Matrix<double,3,Dynamic> positionLocalCS(3,nPoints);
	for(size_t i=0; i<nPoints; i++)
	{
		pteros::Selection select;
		try
		{
			select.modify(system,domain->COMselections.at(i).toStdString());
		}
		catch (const pteros::Pteros_error &err)
		{
			std::cerr<<err.what()<<std::endl;
		}

		if(select.get_index().size()!=1)
		{
			std::cerr<<std::endl;
			std::cerr  << "Specified selection could not be mapped correctly: " <<
				      domain->COMselections.at(i).toStdString() << std::endl;
			matrix = Eigen::Matrix4d().setConstant(
					 std::numeric_limits<double>::quiet_NaN());
			return std::make_shared<CalcResult<Eigen::Matrix4d>>(std::move(matrix));
		}

		positionGlobalCS.col(i)=select.XYZ(0).cast<double>()*10.0;
		positionLocalCS.col(i)=domain->COMpositionLocalCS.at(i);

	}
	matrix=Eigen::umeyama(positionLocalCS,positionGlobalCS,false);

	return std::make_shared<CalcResult<Eigen::Matrix4d>>(std::move(matrix));
}
