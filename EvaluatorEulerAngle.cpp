#include "EvaluatorEulerAngle.h"
#include "CalcResult.h"

#include <async++.h>


AbstractEvaluator::Task EvaluatorEulerAngle::makeTask(const FrameDescriptor &frame) const
{
	//get tasks for ref and body;
	Task refTask=getTask(frame,_refCalc,false);
	Task bodyTask=getTask(frame,_bodyCalc,false);
	using result_t=std::tuple<Task,Task>;
	return async::when_all(bodyTask,refTask).then(
				[this](result_t result){
		auto ptrBody=std::get<0>(result).get();
		auto ptrRef=std::get<1>(result).get();
		auto resRef=dynamic_cast<CalcResult<Eigen::Matrix4d>*>(ptrRef.get());
		auto resBody=dynamic_cast<CalcResult<Eigen::Matrix4d>*>(ptrBody.get());
		Eigen::Matrix4d ref=resRef->get();
		Eigen::Matrix4d body=resBody->get();
		return calculate(ref, body);
	}).share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorEulerAngle::calculate( const Eigen::Matrix4d& ref,
			       const Eigen::Matrix4d& body) const
{
	const Eigen::Matrix3d& Rrd=ref.block<3,3>(0,0);
	const Eigen::Matrix3d& Rd=body.block<3,3>(0,0);
	Eigen::Vector3d angles=(Rrd.transpose()*Rd).eulerAngles(2,1,2)*(180.0/3.141592653589793);
	if(angles(1)<0.0)
	{
		angles={angles(0)+180.0,-1.0*angles(1),angles(2)+180.0};
		angles[0]-=angles[0]>180.0?360.0:0.0;
		angles[2]-=angles[2]>180.0?360.0:0.0;
	}
	return std::make_shared<CalcResult<Eigen::Vector3d>>(std::move(angles));
}
