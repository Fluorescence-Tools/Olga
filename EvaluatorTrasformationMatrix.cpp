#include <iostream>

#include <Eigen/Core>
#include <Eigen/Dense>

#include "EvaluatorTrasformationMatrix.h"
#include "CalcResult.h"


EvaluatorTrasformationMatrix::
EvaluatorTrasformationMatrix(const TaskStorage& storage, const std::string &name) :
	AbstractEvaluator(storage),_name(name)
{
	comSellPos.resize(3,std::make_pair("",Eigen::Vector3d(0.0,0.0,0.0)));
}

AbstractEvaluator::Task
EvaluatorTrasformationMatrix::makeTask(const FrameDescriptor &frame) const
{
	auto sysTask=getSysTask(frame);
	return sysTask.then([this](pteros::System system) {
		return calculate(system);
	}).share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorTrasformationMatrix::calculate(const pteros::System& system) const
{
	//auto system=getSystem(desc);
	Eigen::Matrix4d matrix;

	using Eigen::Dynamic;
	Eigen::Matrix<double,3,Dynamic> positionGlobalCS(3,numPoints());
	Eigen::Matrix<double,3,Dynamic> positionLocalCS(3,numPoints());
	for(int i=0; i<numPoints(); i++)
	{
		pteros::Selection select;
		try
		{
			select.modify(system,comSellPos[i].first);
		}
		catch (const pteros::Pteros_error &err)
		{
			std::cerr<<err.what()<<std::endl;
		}

		if(select.get_index().size()!=1)
		{
			std::cerr<<std::endl;
			std::cerr  << "Specified selection could not be mapped correctly: " <<
				      comSellPos[i].first << std::endl;
			matrix = Eigen::Matrix4d().setConstant(
					 std::numeric_limits<double>::quiet_NaN());
			return std::make_shared<CalcResult<Eigen::Matrix4d>>(std::move(matrix));
		}

		positionGlobalCS.col(i)=select.XYZ(0).cast<double>()*10.0;
		positionLocalCS.col(i)=comSellPos[i].second;

	}
	matrix=Eigen::umeyama(positionLocalCS,positionGlobalCS,false);
	return std::make_shared<CalcResult<Eigen::Matrix4d>>(std::move(matrix));
}
