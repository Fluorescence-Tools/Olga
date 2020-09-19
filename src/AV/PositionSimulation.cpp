#include "PositionSimulation.h"
#include "fretAV.h"
#include "Position.h"
#include <QTextStream>
#include <QFile>
#include <QCoreApplication>

PositionSimulation::PositionSimulation()
{
}

PositionSimulation::~PositionSimulation()
{
}

PositionSimulation *
PositionSimulation::create(const Position::SimulationType &simulationType)
{
	switch (simulationType) {
	case Position::SimulationType::AV1:
		return new PositionSimulationAV1;
	case Position::SimulationType::AV3:
		return new PositionSimulationAV3;
	case Position::SimulationType::ATOM:
		return new PositionSimulationAtom;
	}
	return nullptr;
}

PositionSimulation::Setting PositionSimulationAV3::setting(int row) const
{
	switch (row) {
	case 0:
		return Setting{"simulation_grid_resolution", gridResolution};
	case 1:
		return Setting{"linker_length", linkerLength};
	case 2:
		return Setting{"linker_width", linkerWidth};
	case 3:
		return Setting{"radius1", radius[0]};
	case 4:
		return Setting{"radius2", radius[1]};
	case 5:
		return Setting{"radius3", radius[2]};
	case 6:
		return Setting{"min_sphere_volume_fraction",
			       minVolumeSphereFraction};
	case 7:
		return Setting{"contact_volume_thickness", contactR};
	case 8:
		return Setting{"contact_volume_trapped_fraction", trappedFrac};
	}
	return Setting();
}

void PositionSimulationAV3::setSetting(int row, const QVariant &val)
{
	switch (row) {
	case 0:
		gridResolution = val.toDouble();
		return;
	case 1:
		linkerLength = val.toDouble();
		return;
	case 2:
		linkerWidth = val.toDouble();
		return;
	case 3:
		radius[0] = val.toDouble();
		return;
	case 4:
		radius[1] = val.toDouble();
		return;
	case 5:
		radius[2] = val.toDouble();
		return;
	case 6:
		minVolumeSphereFraction = val.toDouble();
		return;
	case 7:
		contactR = val.toDouble();
		return;
	case 8:
		trappedFrac = val.toDouble();
		return;
	}
}

PositionSimulationResult
PositionSimulation::calculate(const Eigen::Vector3f &attachmentAtomPos,
			      const std::vector<Eigen::Vector4f> &xyzW)
{
	Eigen::Vector4f v(attachmentAtomPos(0), attachmentAtomPos(1),
			  attachmentAtomPos(2), 0.0f);
	int atom_i = -1;
	for (unsigned i = 0; i < xyzW.size(); i++) {
		v[3] = xyzW[i][3];
		if ((v - xyzW.at(i)).squaredNorm() < 0.01f) {
			atom_i = i;
			break;
		}
	}
	if (atom_i < 0) {
		std::cerr
			<< "ERROR! Attachment atom is not found in the stripped molecule\n"
			<< std::flush;
		return PositionSimulationResult();
	}
	// TODO: this is a horrible hack
	std::vector<Eigen::Vector4f> xyzW2 = xyzW;
	xyzW2[atom_i][3] = 0.0f;
	return calculate(atom_i, xyzW2);
}

PositionSimulationResult
PositionSimulationAV3::calculate(unsigned atom_i,
				 const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector4f> res =
		calculateAV3(xyzW, xyzW[atom_i], linkerLength, linkerWidth,
			     {radius[0], radius[1], radius[2]}, gridResolution,
			     contactR, trappedFrac);
	double volfrac = res.size()
			 / (4.0 / 3.0 * 3.14159
			    * std::pow(linkerLength / gridResolution, 3.0));
	if (minVolumeSphereFraction > volfrac) {
		res.clear();
	}
	return PositionSimulationResult(std::move(res));
}

PositionSimulation::Setting PositionSimulationAV1::setting(int row) const
{
	switch (row) {
	case 0:
		return Setting{"simulation_grid_resolution", gridResolution};
	case 1:
		return Setting{"linker_length", linkerLength};
	case 2:
		return Setting{"linker_width", linkerWidth};
	case 3:
		return Setting{"radius1", radius};
	case 4:
		return Setting{"min_sphere_volume_fraction",
			       minVolumeSphereFraction};
	case 5:
		return Setting{"contact_volume_thickness", contactR};
	case 6:
		return Setting{"contact_volume_trapped_fraction", trappedFrac};
	case 7:
		return Setting{"chain_weighting", chainWeighting};
	}
	return Setting();
}

void PositionSimulationAV1::setSetting(int row, const QVariant &val)
{
	switch (row) {
	case 0:
		gridResolution = val.toDouble();
		return;
	case 1:
		linkerLength = val.toDouble();
		setWeightingFunction();
		return;
	case 2:
		linkerWidth = val.toDouble();
		return;
	case 3:
		radius = val.toDouble();
		return;
	case 4:
		minVolumeSphereFraction = val.toDouble();
		return;
	case 5:
		contactR = val.toDouble();
		return;
	case 6:
		trappedFrac = val.toDouble();
		return;
	case 7:
		chainWeighting = val.toBool();
		setWeightingFunction();
		return;
	}
}

PositionSimulationResult
PositionSimulationAV1::calculate(unsigned atom_i,
				 const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector4f> res;
	res = calculateAV(xyzW, xyzW[atom_i], linkerLength, linkerWidth, radius,
			  gridResolution, contactR, trappedFrac,
			  weightingFunction);
	double volfrac = res.size()
			 / (4.0 / 3.0 * 3.14159
			    * std::pow(linkerLength / gridResolution, 3.0));
	if (minVolumeSphereFraction > volfrac) {
		res.clear();
	}
	return PositionSimulationResult(std::move(res));
}
std::map<double, TabulatedFunction> PositionSimulationAV1::weightingFunctions{};
std::map<double, TabulatedFunction>
PositionSimulationAV1::loadWeightingFunctions()
{
	std::map<double, TabulatedFunction> functions;
	Eigen::VectorXd y0 = Eigen::VectorXd::Constant(2, 1.0);
	// default function
	functions[0.0] =
		TabulatedFunction(0.0, std::numeric_limits<double>::max(), y0);

	QString path = QCoreApplication::applicationDirPath()
		       + "/weighting_function.csv";
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		std::cerr << "could not open " + path.toStdString() + "\n"
			  << std::flush;
		return functions;
	}
	std::vector<double> lengthList;
	QStringList lines =
		QTextStream(&file).readAll().remove('\r').split('\n');
	QStringList wordList = lines.first().split('\t');
	wordList.removeFirst(); // X column
	for (const QString &word : wordList) {
		bool ok = false;
		double length = word.toDouble(&ok);
		if (ok) {
			lengthList.push_back(length);
		} else {
			std::cerr
				<< "wrong file format. Header must only contain real numbers."
					   + path.toStdString() + "\n"
				<< std::flush;
			return functions;
		}
	}
	lines.removeFirst(); // header
	if (lines.last().isEmpty()) {
		lines.removeLast();
	}
	Eigen::MatrixXd y =
		Eigen::MatrixXd::Zero(lines.size(), lengthList.size());
	Eigen::VectorXd x(lines.size());
	for (int row = 0; row < y.rows(); ++row) {
		QString &line = lines[row];
		QTextStream in(&line);
		in >> x[row];
		for (int c = 0; c < y.cols(); ++c) {
			in >> y(row, c);
		}
	}
	double xMin = x.minCoeff();
	double xMax = x.maxCoeff();
	if (xMin != x[0] || xMax != x[x.size() - 1]) {
		std::cerr
			<< "Wrong file format, Rda values do not increase monotonically. "
				   + path.toStdString() + "\n"
			<< std::flush;
		return functions;
	}
	for (int col = 0; col < y.cols(); ++col) {
		functions[lengthList[col]] =
			TabulatedFunction(xMin, xMax, y.col(col));
	}
	return functions;
}

void PositionSimulationAV1::setWeightingFunction() const
{
	if (!chainWeighting) {
		weightingFunction = weightingFunctions.at(0.0);
		return;
	}
	auto it = weightingFunctions.lower_bound(linkerLength);
	if (it == weightingFunctions.end()) {
		weightingFunction = weightingFunctions.at(0);
		std::cerr
			<< "could not find a weighting fucntion for linker length of "
				   + std::to_string(linkerLength)
				   + " Angstrom.\n"
			<< std::flush;
		return;
	}
	weightingFunction = it->second;
}
