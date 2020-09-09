#include <fstream>
#include <iomanip>
#include <future>
#include <sstream>

#include <QProgressDialog>
#include <QFileDialog>
#include <QTime>

#include <pteros/pteros.h>

#include "GetInformativePairsDialog.h"
#include "ui_GetInformativePairsDialog.h"
#include "best_dist.h"
#include "center.h"
#include "TaskStorage.h"

using namespace std::string_literals;

GetInformativePairsDialog::GetInformativePairsDialog(
	QWidget *parent, const std::vector<FrameDescriptor> &frames,
	const Eigen::MatrixXf &effs, const std::vector<std::string> &evalNames,
	const TaskStorage &storage)
    : QDialog(parent), frames(frames), effs(effs), storage(storage),
      evalNames(evalNames), ui(new Ui::GetInformativePairsDialog)
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
	const int numFrames = frames.size();
	System traj;
	traj.load(frames[0].topologyFileName());
	if (traj.select(sel).size() == 0) {
		std::cerr << "ERROR! Selection <"s + sel + "> found 0 atoms!\n"
			  << std::flush;
		return pteros::System();
	}
	try {
		traj.keep(sel);

	} catch (std::exception &e) {
		std::cerr << "EXCEPTION! "s + e.what() + "\n" << std::flush;
		return pteros::System();
	}
	for (int i = 1; i < numFrames; ++i) {
		const FrameDescriptor &fr = frames[i];
		const auto &sysTsk = storage.getSysTask(fr);

		if (i + 1 < numFrames) {
			// prefetch:
			storage.getSysTask(frames[i + 1]);
		}

		System system = sysTsk.get();
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

	future<pteros::System> fTraj = async(std::launch::async, [this, sel] {
		return buildTrajectory(sel);
	});
	showProgress("Building trajectory...", fTraj);
	pteros::System traj = fTraj.get();
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

	auto fRMSDs = async(std::launch::async,
			    [this, &traj] { return rmsd2d(traj, fracDone); });
	showProgress("Calculating RMSD...", fRMSDs);
	const MatrixXf &RMSDs = fRMSDs.get();
	if (RMSDs.hasNaN()) {
		std::cerr << "RMSDs has NaN!\n" << std::flush;
		return;
	}

	std::vector<unsigned> keepIdxs = thresholdNansPerCol(effs, 0.2);
	std::vector<std::string> pairNames;
	for (unsigned i : keepIdxs) {
		pairNames.push_back(evalNames[i]);
	}

	// Cleanup Efficiencies
	auto fCleanup = async(std::launch::async, [&] {
		MatrixXf E = sliceCols(effs, keepIdxs);
		fillNans(E, RMSDs);
		return E;
	});
	fracDone = -1.0f;
	showProgress("Cleaning up NaNs...", fCleanup);
	const MatrixXf &E = fCleanup.get();


	const bool uniqueOnly = ui->uniqueOnly->isChecked();
	const int maxPairs = std::min(int(E.cols()), numPairsMax);

	auto fPairs = async(std::launch::async, [&] {
		return greedySelection(err, E, RMSDs, maxPairs, fracDone,
				       uniqueOnly);
	});
	showProgress("Greedy selection...", fPairs);
	std::vector<unsigned> pairIdxs = fPairs.get();


	auto fDecay = async(std::launch::async, [&] {
		return precisionDecay(pairIdxs, E, RMSDs, err);
	});
	fracDone = -1.0f;
	showProgress("Generating report...", fDecay);
	Eigen::VectorXf rmsdAve = fDecay.get();

	std::ostringstream report;
	report << "#\tPair_added\t<<RMSD>>/A\n";
	report << std::fixed << std::setprecision(2);
	// colwise() helps preventing float overflow
	const float rmsdMeanInit = RMSDs.colwise().mean().mean();
	report << "0\t--\t" << rmsdMeanInit << "\n";
	for (int i = 0; i < pairIdxs.size(); ++i) {
		report << i + 1 << "\t" << pairNames[pairIdxs[i]] << "\t"
		       << rmsdAve[i] << "\n";
	}

	if (pairIdxs.size() == 0) {
		report.clear();
		report << "No NaN-free pairs found. Pair selection failed.\n";
	}

	std::string fname = ui->fileEdit->text().toStdString();
	if (fname.empty()) {
		std::cout << report.str() << std::flush;
	} else {
		std::ofstream outfile(fname, std::ifstream::out);
		if (!outfile.is_open()) {
			std::cerr << "Warning! could not open file for saving: "
					     + fname + "\n"
				  << std::flush;
			return;
		}
		outfile << report.str();
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

template <typename T>
void GetInformativePairsDialog::showProgress(const QString &title,
					     const std::future<T> &future)
{
	// auto future = std::async(std::launch::async, func);
	using namespace std::chrono;
	QProgressDialog dialog(this);
	dialog.setLabelText(title);
	dialog.setWindowTitle(title);
	dialog.setWindowModality(Qt::WindowModal);
	if (fracDone < 0.0f) {
		dialog.setRange(0, 0);
	} else {
		dialog.setRange(0, 1000);
	}
	dialog.setMinimumDuration(0);
	dialog.setCancelButton(0);

	Qt::WindowFlags flags = dialog.windowFlags();
	flags |= Qt::CustomizeWindowHint;
	flags &= ~Qt::WindowCloseButtonHint;
	dialog.setWindowFlags(flags);

	dialog.show();

	auto updInterval = std::chrono::milliseconds(20);
	auto start = std::chrono::system_clock::now();
	while (future.wait_for(updInterval) != std::future_status::ready) {
		auto now = system_clock::now();
		double dt_s =
			duration_cast<duration<double>>(now - start).count();
		int remaining_seconds = dt_s / (fracDone + 0.001f) - dt_s;
		QString eta = "\nEstimated runtime remaining: ";
		if (remaining_seconds > 60 * 60 * 24) {
			int nDays = remaining_seconds / (60 * 60 * 24);
			eta += QString("%1d ").arg(nDays);
		}
		eta += QTime(0, 0)
			       .addSecs(remaining_seconds)
			       .toString("hh:mm:ss");
		if (fracDone > 0.0f) {
			dialog.setLabelText(title + eta);
			dialog.setValue(int(fracDone * 1000.0f));
		}
		// setValue() calls processEvents() only if percDone has changed
		QApplication::processEvents(); // makes GUI more responsive
	}
	dialog.close();
}
