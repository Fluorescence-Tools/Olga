#include "GetInformativePairsDialog.h"
#include "ui_GetInformativePairsDialog.h"
#include "best_dist.h"
#include "theobald_rmsd.h"
#include "center.h"
#include <fstream>
#include <iomanip>
#include <pteros/pteros.h>
#include <QProgressDialog>
#include <QFileDialog>
#include <future>

GetInformativePairsDialog::GetInformativePairsDialog(
	QWidget *parent, const std::vector<FrameDescriptor> &frames,
        const Eigen::MatrixXf &effs, const std::vector<std::string> &evalNames)
    : QDialog(parent), frames(frames), effs(effs), evalNames(evalNames),
      ui(new Ui::GetInformativePairsDialog)
{
	ui->setupUi(this);
}

GetInformativePairsDialog::~GetInformativePairsDialog()
{
	delete ui;
}

void GetInformativePairsDialog::setMaxPairs(int numPairsMax)
{
	ui->maxPairs->setValue(numPairsMax);
}

void GetInformativePairsDialog::setError(float err)
{
	ui->errorSpinBox->setValue(err);
}

void GetInformativePairsDialog::setOutFile(const QString &path)
{
	ui->fileEdit->setText(path);
}

pteros::System
GetInformativePairsDialog::buildTrajectory(const std::string &sel) const
{
	using namespace pteros;
	const size_t numFrames = frames.size();
	System traj;
	traj.load(frames[0].topologyFileName());
	traj.keep(sel);
	for (int i = 1; i < numFrames; ++i) {
		const FrameDescriptor &fr = frames[i];
		System system(fr.topologyFileName());
		system.keep(sel);
		if (traj.num_atoms() == system.num_atoms()) {
			traj.frame_append(system.frame(0));
		} else {
			std::cerr
			        << "ERROR! Number of atoms does not match "
					   + fr.topologyFileName() + " atoms: "
					   + std::to_string(system.num_atoms())
					   + "/"
					   + std::to_string(traj.num_atoms())
					   + "\n"
				<< std::flush;
		}
		fracDone = float(i) / float(numFrames);
	}
	if (numFrames != traj.num_frames()) {
		std::cerr << "ERROR! loaded "
		                     + std::to_string(traj.num_frames())
		                     + " out of " + std::to_string(numFrames)
		                     + " frames for pair selection.\n"
			  << std::flush;
	}
	return traj;
}

void GetInformativePairsDialog::accept()
{
	using Eigen::MatrixXf;
	using std::async;
	using std::future;
	using std::map;
	using std::string;

	const std::string sel = ui->selectionEdit->text().toStdString();
	const double err = ui->errorSpinBox->value();
	const int numPairsMax = ui->maxPairs->value();
	const int numFitParams = 0;


	pteros::System traj;
	showProgress("Building trajectory...",
		     [&] { traj = buildTrajectory(sel); });
	const size_t numFrames = traj.num_frames();
	if (numFrames < 2) {
		std::cerr
		        << "ERROR! Number of frames in trajectory is less than 2!";
		return;
	}
	if (numFrames != effs.rows()) {
		std::cerr
		        << "ERROR! Number of frames in trajectory does not match the number of rows in Efficiency matrix!";
		return;
	}

	MatrixXf RMSDs;
	showProgress("Calculating RMSD...",
	             [&] { RMSDs = rmsd2d(traj, fracDone); });

	if (RMSDs.hasNaN()) {
		std::cerr << "RMSDs has NaN!\n" << std::flush;
		return;
	}

	// Cleanup Efficiencies
	std::vector<std::string> pairNames;
	const double maxNanFraction = 0.2;
	const int maxNan = int(maxNanFraction * effs.rows());
	Eigen::VectorXi numNanCol =
	        effs.array().isNaN().cast<int>().colwise().sum();
	std::vector<unsigned> keepIdxs;
	for (unsigned i = 0; i < effs.cols(); ++i) {
		if (numNanCol[i] < maxNan) {
			keepIdxs.push_back(i);
			pairNames.push_back(evalNames[i]);
		}
	}
	MatrixXf E = sliceCols(effs, keepIdxs);
	fillNans(E, RMSDs);

	const int maxPairs = std::min(int(E.cols()), numPairsMax);
	std::vector<unsigned> pairIdxs;
	showProgress("Greedy selection...", [&]() {
		pairIdxs =
			greedySelection(err, E, RMSDs, maxPairs, numFitParams);
	});

	Eigen::VectorXf rmsdAve = precisionDecay(pairIdxs, E, RMSDs, err);
	std::string report = "#\tPair_added\t<<RMSD>>/A\n";
	for (int i = 0; i < pairIdxs.size(); ++i) {
		report += std::to_string(i + 1) + "\t" + pairNames[pairIdxs[i]]
		          + "\t" + std::to_string(rmsdAve[i]) + "\n";
	}

	if (pairIdxs.size() == 0) {
		report = "No NaN-free pairs found. Pair selection failed.\n";
	}

	std::string fname = ui->fileEdit->text().toStdString();
	if (fname.empty()) {
		std::cout << report << std::flush;
	} else {
		std::ofstream outfile(fname, std::ifstream::out);
		if (!outfile.is_open()) {
			std::cerr << "Warning! could not open file for saving: "
					     + fname + "\n"
				  << std::flush;
			return;
		}
		outfile << report;
	}

	QDialog::accept();
}


void GetInformativePairsDialog::setFileName()
{
	QString fileName = QFileDialog::getSaveFileName(
		this, tr("report file"), "",
		tr("Text file (*.txt);;Any file (*)"));
	ui->fileEdit->setText(fileName);
}

template <typename Lambda>
void GetInformativePairsDialog::showProgress(const QString &title,
					     Lambda &&func)
{
	fracDone = 0.0f;
	QProgressDialog dialog(title, QString(), 0, 1000, this);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.setMinimumDuration(0);
	auto f = std::async(std::launch::async, func);
	std::future_status status = f.wait_for(std::chrono::milliseconds(0));
	while (status != std::future_status::ready) {
		// setValue() calls processEvents() only if percDone has changed
		dialog.setValue(int(fracDone * 1000.0f));
		QApplication::processEvents(); // makes GUI more responsive
		status = f.wait_for(std::chrono::milliseconds(20));
	}
	f.get();
	dialog.setValue(1000);
}

std::vector<unsigned> GetInformativePairsDialog::greedySelection(
	const float err, const Eigen::MatrixXf &Effs,
	const Eigen::MatrixXf &RMSDs, const int maxPairs,
	const int fitParams) const
{
	std::vector<unsigned> selPairs;
	for (int pairsDone = 0; pairsDone < maxPairs; ++pairsDone) {
		unsigned best = bestPair(Effs, RMSDs, err, 0.99f, selPairs);
		selPairs.push_back(best);
		fracDone = 100 * pairsDone / maxPairs;
	}
	return selPairs;
}
