#include "EvaluatorPositionSimulation.h"
#include "CalcResult.h"

std::shared_ptr<AbstractCalcResult> EvaluatorPositionSimulation::calculate(const pteros::System &system,const FrameDescriptor &frame) const
{
	PositionSimulationResult res=_position.calculate(system);
	if(res.empty()) {
		std::cerr<<frame.fullName()+" simulation "+_position.name()+" failed: empty AV\n";
		std::cerr.flush();
	}
	/*std::string fname=_position.name();
	std::replace(fname.begin(),fname.end(),'/','_');
	res.dumpShellXyz(frame.trajFileName()+"_"+fname+".xyz");
	std::cout<<"dumping! "+frame.trajFileName()+"_"+fname+".xyz\n"<<std::flush;*/
	return std::make_shared<CalcResult<PositionSimulationResult>>(std::move(res));
}

EvaluatorPositionSimulation::
EvaluatorPositionSimulation(const TaskStorage& storage,
			    const QVariantMap& settings, const std::string &name):
	AbstractEvaluator(storage),_position(settings,name)
{
}

AbstractEvaluator::Task EvaluatorPositionSimulation::makeTask(const FrameDescriptor &frame) const noexcept
{
	auto sysTask=getSysTask(frame);
	return sysTask.then([this,frame](pteros::System system) {
		return calculate(system,frame);
	}).share();
}
