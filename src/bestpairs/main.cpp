#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

#include <QCommandLineParser>
#include <QDir>
#include <QString>
#include <QSet>
#include <QVector>

#include <pteros/pteros.h>

#include "best_dist.h"

using namespace std;

pteros::System loadTrajectory(const QString &dirPath,
                              const vector<string> &basenames)
{
	pteros::System traj;
	const string sel = "name CA";

	QDir dir(dirPath);
	QSet<QString> pdbs = dir.entryList(QStringList() << "*.pdb"
                                                         << "*.PDB",
                                           QDir::Files)
                                     .toSet();
	for (const string &pdb : basenames) {
		if (!pdbs.contains(QString::fromStdString(pdb))) {
			std::cerr << "ERROR! " + pdb + " was not found in "
                                             + dirPath.toStdString() + "\n";
			return pteros::System();
		}
	}
	QString file = QString::fromStdString(basenames[0]);
	traj.load(dir.absoluteFilePath(file).toStdString());
	traj.keep(sel);
	const int numAt = traj.num_atoms();
	for (size_t i = 1; i < basenames.size(); ++i) {
		file = QString::fromStdString(basenames[i]);
		string path = dir.absoluteFilePath(file).toStdString();
		pteros::System sys(path);
		sys.keep(sel);
		if (numAt != sys.num_atoms()) {
			cerr << "ERROR! Number of atoms in the file does not match the previous frames: "
                                        + path + "\n";
			return pteros::System();
		}
		traj.frame_append(sys.frame(0));
	}
	return traj;
}
Eigen::MatrixXf loadDistanceMatrix(const std::string &path,
                                   const vector<string> &refnames)
{
	ifstream indata(path);
	if (!indata.is_open()) {
		cerr << "ERROR! Could not open file: " + path + "\n";
		return Eigen::MatrixXf();
	}
	string line;
	getline(indata, line);
	if (line.back() == '\t') {
		line.pop_back();
	}

	// first column is the conformer name
	int numConf = count(line.begin(), line.end(), '\t');
	if (line[0] != '#') {
		indata.clear();
		indata.seekg(0);
	}
	Eigen::MatrixXf dm(numConf, numConf);
	int row;
	for (row = 0; getline(indata, line) && (row < numConf); ++row) {
		stringstream lineStream(line);
		string confName;
		getline(lineStream, confName, '\t');
		if (confName != refnames[row]) {
			cerr << "ERROR! Order of conformers does not match to the reference: "
                                        + path + "\n";
			return Eigen::MatrixXf();
		}

		string tmp;
		int col;
		for (col = 0; lineStream.good() && (col < numConf); ++col) {
			getline(lineStream, tmp, '\t');
			dm(row, col) = stof(tmp);
		}
		if (col != numConf) {
			cerr << "ERROR! Number of elements in the row does not match the number of conformers: "
                                        + path + "\n";
			return Eigen::MatrixXf();
		}
	}
	if (row != numConf) {
		cerr << "ERROR! Number of rows in the file does not match the number of conformers: "
                                + path + "\n";
		return Eigen::MatrixXf();
	}
	return dm;
}
struct EfficiencyTable {
	EfficiencyTable(const std::string &path);
	vector<string> columnNames, conformerNames;
	Eigen::MatrixXf E;
	void fixNans(double maxNanFraction, const Eigen::MatrixXf &RMSDs);
	void keepConfs(const QSet<QString> &validNames);
};

EfficiencyTable::EfficiencyTable(const string &path)
{
	ifstream indata(path);
	if (!indata.is_open()) {
		cerr << "ERROR! Could not open file: " + path + "\n";
		return;
	}
	string line;

	// columnNames
	getline(indata, line);
	stringstream lineStream(line);
	string tmp;
	getline(lineStream, tmp, '\t'); // skip 'structure'
	while (lineStream.good()) {
		getline(lineStream, tmp, '\t');
		if (tmp.size() > 0) {
			columnNames.push_back(tmp);
		}
	}
	const size_t numCols = columnNames.size();

	vector<float> vals;
	while (getline(indata, line)) {
		stringstream lineStream(line);
		string confName;
		getline(lineStream, confName, '\t');
		if (confName.substr(confName.size() - 3) == " #0") {
			confName.erase(confName.size() - 3);
		}
		conformerNames.push_back(confName);
		size_t numVals = 0;
		while (lineStream.good()) {
			getline(lineStream, tmp, '\t');
			if (tmp.size() > 0) {
				vals.push_back(stof(tmp));
				++numVals;
			}
		}
		if (numVals != numCols) {
			cerr << "ERROR! Number of elements in the row does not match the number of columns: "
                                        + path + "\n";
			return;
		}
	}
	const size_t numRows = conformerNames.size();
	assert(numRows * numCols == vals.size());
	using MatrixXfRM = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic,
                                         Eigen::RowMajor>;
	E = Eigen::Map<MatrixXfRM>(vals.data(), numRows, numCols);
}

void EfficiencyTable::fixNans(double maxNanFraction,
                              const Eigen::MatrixXf &RMSDs)
{
	std::vector<unsigned> keepIdxs = thresholdNansPerCol(E, 0.2);
	std::vector<std::string> keepNames;
	for (unsigned i : keepIdxs) {
		keepNames.push_back(columnNames[i]);
	}
	columnNames = std::move(keepNames);
	Eigen::MatrixXf slice = sliceCols(E, keepIdxs);
	E = slice;
	fillNans(E, RMSDs);
}

void EfficiencyTable::keepConfs(const QSet<QString> &validNames)
{
	std::vector<int> validIdxs;
	for (size_t i = 0; i < conformerNames.size(); ++i) {
		QString conf = QString::fromStdString(conformerNames[i]);
		if (validNames.contains(conf)) {
			validIdxs.push_back(i);
		}
	}
	// E = E(validIdxs, Eigen::all).eval(); //Since Eigen 3.4
	const size_t rows = validIdxs.size();
	Eigen::MatrixXf res(rows, E.cols());
	vector<string> keepConfs;
	keepConfs.reserve(rows);
	for (size_t i = 0; i < rows; ++i) {
		size_t validx = validIdxs[i];
		res.row(i) = E.row(validx);
		keepConfs.emplace_back(std::move(conformerNames[validx]));
	}
	E = res;
	conformerNames = std::move(keepConfs);
}

int main(int argc, char *argv[])
{
	pteros::set_log_level("off");
	QCoreApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	QCommandLineParser parser;
	parser.addOptions({
                {"data", "a file containing FRET efficiencies for an ensmeble",
                 "path"},
                {{"dir", "d"}, "a directory with pdb files (for RMSD)", "path"},
                {"numpairs", "number of pairs to select", "integer"},
                {"err", "Efficiency error to assume for pair selection",
                 "float"},
                {"savepairs", "save selected pairs", "path"},
                {"distance-matrix",
                 "matrix of distances between conformers (e.g. RMSD)", "path"},
	});
    parser.addHelpOption();
    parser.addVersionOption();
	parser.process(a);

	QString effsPath = parser.value("data");
	QString pdbsDirPath = parser.value("dir");
	int numSelPairs = parser.value("numpairs").toInt();
	float err = parser.value("err").toFloat();
	QString pairsPath = parser.value("savepairs");
	QString distMatPath = parser.value("distance-matrix");

	// effs
	EfficiencyTable effs(effsPath.toStdString());

	if (!pdbsDirPath.isEmpty()) {
		QDir pdbsDir(pdbsDirPath);
		QSet<QString> pdbs = pdbsDir.entryList(QStringList() << "*.pdb"
                                                                     << "*.PDB",
                                                       QDir::Files)
                                             .toSet();
		effs.keepConfs(pdbs);
	}
	if (effs.conformerNames.size() < 2) {
		cerr << "Error! Too litle conformers were loaded. At least 2 are needed.\n";
		return 1;
	}

	Eigen::MatrixXf distMat;
	if (distMatPath.isEmpty()) {
		// rmsds
		pteros::System traj =
                        loadTrajectory(pdbsDirPath, effs.conformerNames);
		if (traj.num_frames() == 0) {
			cerr << "Error! could not load the pdbs: "
                                        + pdbsDirPath.toStdString() + "\n";
			return 2;
		}
		distMat = rmsd2d(traj);
	} else {
		// External dissmilarity (distance) matrix.
		distMat = loadDistanceMatrix(distMatPath.toStdString(),
                                             effs.conformerNames);
		if (effs.E.rows() != distMat.rows()) {
			cerr << "Number of rows in efficiency table does not match to the number of rows in distance matrix: "
                                        + to_string(effs.E.rows()) + " != "
                                        + to_string(distMat.rows()) + "\n";
			return 3;
		}
	}

	effs.fixNans(0.2, distMat);

	// decay
	vector<unsigned> pairIdxs;
        pairIdxs = greedySelection(err, effs.E, distMat, numSelPairs, true);
	// report
	Eigen::VectorXf rmsdAve =
                precisionDecay(pairIdxs, effs.E, distMat, err);
	string report = "#\tPair_added\t<<RMSD>>/A\n";
	const auto &pairNames = effs.columnNames;
	for (size_t i = 0; i < pairIdxs.size(); ++i) {
		report += std::to_string(i + 1) + "\t" + pairNames[pairIdxs[i]]
                          + "\t" + std::to_string(rmsdAve[i]) + "\n";
	}

	if (pairIdxs.size() == 0) {
		report = "No NaN-free pairs found. Pair selection failed.\n";
	}

	if (pairsPath.isEmpty()) {
		cout << report << flush;
	} else {
		ofstream outfile(pairsPath.toStdString(), std::ifstream::out);
		if (!outfile.is_open()) {
			cerr << "Warning! could not open file for saving: "
                                        + pairsPath.toStdString() + "\n"
			     << flush;
			return 4;
		}
		outfile << report;
	}
	return 0;
}
