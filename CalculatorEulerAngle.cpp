#include "CalculatorEulerAngle.h"
#include "CalcResult.h"


std::shared_ptr<AbstractCalcResult> CalculatorEulerAngle::calculate(const FrameDescriptor &desc) const
{
	auto refMatrix=result(desc,_refCalc);
	auto bodyMatrix=result(desc,_bodyCalc);
	if(refMatrix && bodyMatrix) {
		CalcResult<Eigen::Matrix4d> *pRef, *pBody;
		pRef=dynamic_cast<CalcResult<Eigen::Matrix4d>*>(refMatrix.get());
		pBody=dynamic_cast<CalcResult<Eigen::Matrix4d>*>(bodyMatrix.get());
		if(!pRef || !pBody) {//wrong result type
			std::cerr<<"wrong result type, this should never happen."
				   "Unexpected behaviour in CalculatorEulerAngle::calculate."<<std::endl;
			return std::shared_ptr<AbstractCalcResult>();
		}
		const Eigen::Matrix3d& Rrd=pRef->get().block<3,3>(0,0);
		const Eigen::Matrix3d& Rd=pBody->get().block<3,3>(0,0);
		Eigen::Vector3d angles=(Rrd.transpose()*Rd).eulerAngles(2,1,2)*(180.0/3.141592653589793);
		if(angles(1)<0.0)
		{
			angles={angles(0)+180.0,-1.0*angles(1),angles(2)+180.0};
			angles[0]-=angles[0]>180.0?360.0:0.0;
			angles[2]-=angles[2]>180.0?360.0:0.0;
		}
		return std::make_shared<CalcResult<Eigen::Vector3d>>(std::move(angles));
	}
	return std::shared_ptr<AbstractCalcResult>();
}
