#include <QFileInfo>

#include "EvaluatorAvFile.h"
#include "CalcResult.h"

AbstractEvaluator::Task EvaluatorAvFile::makeTask(const FrameDescriptor &frame) const noexcept
{
	Task av=getTask(frame,_av,false);
	std::string trajFname=frame.trajFileName();
	std::string posName = _storage.eval(_av).name();
	using result_t=Task;
	return av.then(
				[this,trajFname,posName](result_t result){
		auto ptrAv=result.get();
		auto resAv=dynamic_cast<CalcResult<PositionSimulationResult>*>(ptrAv.get());
		PositionSimulationResult av=resAv->get();
		QFileInfo trajInfo(QString::fromStdString(trajFname));
		QFileInfo _writeDirInfo(QString::fromStdString(_writeDirPath));
		std::string fname;
		if (_writeDirInfo.isAbsolute()) {
			fname=_writeDirPath+'/'
			      +trajInfo.baseName().toStdString()
			      +"_"+posName;
		} else {
			fname=trajInfo.absolutePath().toStdString()
			      +'/'+_writeDirPath
			      +'/'+trajInfo.baseName().toStdString()
			      +"_"+posName;
		}
		return calculate(av,fname);
	}).share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorAvFile::calculate(const PositionSimulationResult &av,
			   const std::string& fname) const
{
	//std::cout<<"Dumping: "+fname+"\n"<<std::flush;
	if (_onlyShell) {
		return std::make_shared<CalcResult<bool>>(av.dumpShellXyz(fname+".xyz"));
	}
	if(_openDX) {
		return std::make_shared<CalcResult<bool>>(av.dump_dxmap(fname+".dx"));
	}
	return std::make_shared<CalcResult<bool>>(av.dumpXyz(fname));
}
