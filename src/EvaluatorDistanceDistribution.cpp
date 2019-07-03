#include <iomanip>

#include "EvaluatorDistanceDistribution.h"
#include "CalcResult.h"
#include <QDir>
#include <QFileInfo>

std::string
EvaluatorDistanceDistribution::fullPath(const std::string &path) const
{
	if (path.empty()) {
		return QDir::current()
		        .filePath(QString::fromStdString("histogram_" + _name
		                                         + ".dat"))
		        .toStdString();
	}
	QFileInfo pathInfo(QString::fromStdString(path));
	if (pathInfo.isAbsolute()) {
		return path;
	}
	// TODO: implement fileselection dialog for settings
	return QDir::currentPath().toStdString() + "/" + path;
}

void EvaluatorDistanceDistribution::writeString(const std::string &str) const
{
	std::unique_lock<std::mutex> lck(writeMtx);
	if (outfile.is_open()) {
		outfile << str << std::flush;
	}
}

bool EvaluatorDistanceDistribution::initOut() const
{
	std::unique_lock<std::mutex> lck(writeMtx);
	if (outfile.is_open())
		return true;

	std::string fileName = fullPath(_histPath);
	outfile = std::ofstream(fileName, std::ifstream::out);
	if (!outfile.is_open()) {
		std::cerr << "could not open the file for writing: <" + fileName
		                     + ">\n";
		return false;
	}

	std::ostringstream header;
	header << "Frame";
	header << std::setprecision(1);
	header << std::fixed;
	for (double d = _distMin + _binSize * 0.5; d < _distMax;
	     d += _binSize) {
		header << '\t' << d;
	}
	header << '\n';
	outfile << header.str();
	return true;
}

EvaluatorDistanceDistribution::EvaluatorDistanceDistribution(
        const TaskStorage &storage, const std::string &name)
    : AbstractEvaluator(storage), _name(name)
{
}

AbstractEvaluator::Task
EvaluatorDistanceDistribution::makeTask(const FrameDescriptor &frame) const
        noexcept
{
	Task av1 = getTask(frame, _av1, false);
	Task av2 = getTask(frame, _av2, false);
	if (!av1.valid() || !av2.valid()) {
		auto res = std::make_shared<CalcResult<double>>(
		        std::numeric_limits<double>::quiet_NaN());
		return async::make_task(
		               std::shared_ptr<AbstractCalcResult>(res))
		        .share();
	}

	// std::string posName1 = _storage.eval(_av1).name();
	// std::string posName2 = _storage.eval(_av2).name();
	std::string trajFname = frame.trajFileName();

	using result_t = std::tuple<Task, Task>;
	return async::when_all(av1, av2)
	        .then([this, trajFname](result_t result) {
		        auto ptrAv1 = std::get<0>(result).get();
			auto ptrAv2 = std::get<1>(result).get();
			auto resAv1 = dynamic_cast<
			        CalcResult<PositionSimulationResult> *>(
			        ptrAv1.get());
			auto resAv2 = dynamic_cast<
			        CalcResult<PositionSimulationResult> *>(
			        ptrAv2.get());
			if (!resAv1 || !resAv2) {
				auto res = std::make_shared<CalcResult<double>>(
				        std::numeric_limits<
				                double>::quiet_NaN());
				return std::shared_ptr<AbstractCalcResult>(res);
			}
			PositionSimulationResult av1 = resAv1->get();
			PositionSimulationResult av2 = resAv2->get();
			return calculate(av1, av2, trajFname);
	        })
	        .share();
}

std::shared_ptr<AbstractCalcResult>
EvaluatorDistanceDistribution::calculate(const PositionSimulationResult &av1,
                                         const PositionSimulationResult &av2,
                                         const std::string traj) const
{
	std::ostringstream buf;
	buf << std::setprecision(4);
	buf << std::fixed;
	buf << traj;
	unsigned numBins = lround((_distMax - _distMin) / _binSize);
	std::vector<double> hist =
	        av1.RdaDist(av2, _distMin, _distMax, numBins);
	for (double freq : hist) {
		buf << '\t' << freq;
	}
	buf << '\n';

	if (!initOut()) {
		return std::make_shared<CalcResult<bool>>(false);
	}
	writeString(buf.str());
	return std::make_shared<CalcResult<bool>>(true);
}
