#include <QFileInfo>

#include "EvaluatorAvFile.h"
#include "CalcResult.h"
#include <QDir>


std::string stripSpecial(const std::string &name)
{
	std::string res = name;
	static const bool forbidden[128] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
	for (char &c : res) {
		if (c < 0 || forbidden[c]) {
			c = '_';
		}
	}
	return res;
}
AbstractEvaluator::Task
EvaluatorAvFile::makeTask(const FrameDescriptor &frame) const noexcept
{
	Task av = getTask(frame, _av, false);
	std::string trajFname = frame.trajFileName();
	const unsigned iFrame = frame.frame();
	std::string posName = _storage.eval(_av).name();
	using result_t = Task;
	return av
		.then([this, trajFname, posName, iFrame](result_t result) {
			if (!result.valid()) {
				auto res = std::make_shared<CalcResult<bool>>(
					false);
				return std::shared_ptr<AbstractCalcResult>(res);
			}
			auto ptrAv = result.get();
			auto resAv = dynamic_cast<
				CalcResult<PositionSimulationResult> *>(
				ptrAv.get());
			PositionSimulationResult av = resAv->get();
			QFileInfo trajInfo(QString::fromStdString(trajFname));
			QFileInfo _writeDirInfo(
				QString::fromStdString(_writeDirPath));
			std::string fname;
			if (_writeDirInfo.isAbsolute()) {
				fname = _writeDirPath + '/'
					+ trajInfo.baseName().toStdString()
					+ "_" + stripSpecial(posName);
			} else {
				fname = trajInfo.absolutePath().toStdString()
					+ '/' + _writeDirPath + '/'
					+ trajInfo.baseName().toStdString()
					+ "_" + stripSpecial(posName);
			}
			if (iFrame > 0) {
				fname += "_" + std::to_string(iFrame);
			}
			QFileInfo fileInfo(QString::fromStdString(fname));
			if (QFileInfo(fileInfo.absolutePath()).isFile()) {
				std::cerr << _writeDirPath
						     + " is not a directory!\n";
				auto res = std::make_shared<CalcResult<bool>>(
					false);
				return std::shared_ptr<AbstractCalcResult>(res);
			}
			if (!fileInfo.absoluteDir().exists()) {
				std::cout
					<< _writeDirPath
						   + " does not exist, creating.\n";
				QDir().mkpath(fileInfo.absolutePath());
			}
			return calculate(av, fname);
		})
		.share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorAvFile::calculate(const PositionSimulationResult &av,
			   const std::string &fname) const
{
	// std::cout<<"Dumping: "+fname+"\n"<<std::flush;
	bool isSaved = false;
	if (_onlyShell) {
		isSaved = av.dumpShellXyz(fname + ".xyz");
	} else if (_openDX) {
		isSaved = av.dump_dxmap(fname + ".dx");
	} else {
		isSaved = av.dumpXyz(fname + ".xyz");
	}
	return std::make_shared<CalcResult<bool>>(isSaved);
}
