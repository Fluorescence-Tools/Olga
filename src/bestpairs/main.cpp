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

pteros::System loadTrajectory(const vector<string> &basenames,
                              const QString &dirPath)
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
			std::cerr
			        << "ERROR! Number of atoms in the file does not match the previous frames: "
			                   + path + "\n";
			return pteros::System();
		}
		traj.frame_append(sys.frame(0));
	}
	return traj;
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
	QCommandLineParser parser;
	parser.addOptions({
	        {"data", "a file containing FRET efficiencies for an ensmeble",
	         "path"},
	        {{"dir", "d"}, "a directory with pdb files (for RMSD)", "path"},
	        {"numpairs", "number of pairs to select", "integer"},
	        {"err", "Efficiency error to assume for pair selection",
	         "float"},
	        {"savepairs", "save selected pairs", "path"},
	});
	parser.process(a);

	QString effsPath = parser.value("data");
	QString pdbsDirPath = parser.value("dir");
	int numSelPairs = parser.value("numpairs").toInt();
	float err = parser.value("err").toFloat();
	QString pairsPath = parser.value("savepairs");

	// effs
	EfficiencyTable effs(effsPath.toStdString());
	QDir pdbsDir(pdbsDirPath);
	QSet<QString> pdbs = pdbsDir.entryList(QStringList() << "*.pdb"
	                                                     << "*.PDB",
	                                       QDir::Files)
	                             .toSet();
	effs.keepConfs(pdbs);
	if (effs.conformerNames.size() < 2) {
		cerr << "Error! Too litle conformers were loaded. At least 2 are needed.\n";
		return 1;
	}
	const auto &pairNames = effs.columnNames;

	// traj
	pteros::System traj = loadTrajectory(effs.conformerNames, pdbsDirPath);
	if (traj.num_frames() == 0) {
		cerr << "Error! could not load the pdbs: "
		                + pdbsDirPath.toStdString() + "\n";
		return 2;
	}
	// rmsds
	Eigen::MatrixXf RMSDs = rmsd2d(traj);
	effs.fixNans(0.2, RMSDs);
	// decay
	vector<unsigned> pairIdxs;
	pairIdxs = greedySelection(err, effs.E, RMSDs, numSelPairs);
	// report
	Eigen::VectorXf rmsdAve = precisionDecay(pairIdxs, effs.E, RMSDs, err);
	string report = "#\tPair_added\t<<RMSD>>/A\n";
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
			return 3;
		}
		outfile << report;
	}
	return 0;
}
