#include "EvaluatorAvFile.h"
#include "CalcResult.h"

AbstractEvaluator::Task EvaluatorAvFile::makeTask(const FrameDescriptor &frame) const noexcept
{
    Task av=getTask(frame,_av,false);
    std::string trajFname=frame.trajFileName();
    using result_t=Task;
    return av.then(
                [this,trajFname](result_t result){
        auto ptrAv=result.get();
        auto resAv=dynamic_cast<CalcResult<PositionSimulationResult>*>(ptrAv.get());
        PositionSimulationResult av=resAv->get();
        std::string fname = _storage.eval(_av).name();
        std::replace(fname.begin(),fname.end(),'/','_');
        fname=_writeDirPath+trajFname+"_"+fname+".xyz";
        return calculate(av,fname);
    }).share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorAvFile::calculate(const PositionSimulationResult &av,
                           const std::string& fname) const
{
    std::cout<<"Dumping: "+fname+"\n"<<std::flush;
    return std::make_shared<CalcResult<bool>>(av.dumpXyz(fname));
}
