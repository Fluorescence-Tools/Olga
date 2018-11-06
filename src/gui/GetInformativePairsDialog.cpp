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

void dump_rmsds(const Eigen::MatrixXf &m, const std::string &fname)
{
	std::ofstream outfile;
	outfile.open(fname, std::ifstream::out);
	if (!outfile.is_open()) {
		return;
	}
	outfile << std::setprecision(3);
	for (int i = 0; i < m.rows(); ++i) {
		outfile << m(i, 0) * 0.1;
		for (int j = 1; j < m.cols(); ++j) {
			outfile << "\t" << m(i, j) * 0.1;
		}
		outfile << "\n";
	}
}

std::vector<float> sys2xyz(const pteros::System &s)
{
	std::vector<float> vec;
	vec.reserve(s.num_atoms() * s.num_frames() * 3);
	for (int fr = 0; fr < s.num_frames(); ++fr) {
		const float *beg = s.frame(fr).coord.data()->data();
		vec.insert(vec.end(), beg, beg + 3 * s.num_atoms());
		/*
		for(int at=0; at<s.num_atoms(); ++at) {
			const float* beg=s.Frame_data(fr).coord.at(at).data();
			vec.insert(vec.end(),beg,beg+3);
		}*/
	}
	return vec;
}

Eigen::MatrixXf
GetInformativePairsDialog::rmsds(const pteros::System &traj) const
{
	const int numFrames = traj.num_frames();
	const int nAtoms = traj.num_atoms();
	std::vector<float> xyz = sys2xyz(traj);
	std::vector<float> traces(numFrames);
	inplace_center_and_trace_atom_major(xyz.data(), traces.data(),
					    numFrames, nAtoms);
	Eigen::MatrixXf RMSDs(numFrames, numFrames);

	const int64_t numRmsds = int64_t(numFrames) * numFrames / 2;
	std::atomic<std::int64_t> rmsdsDone{0};

	std::vector<std::thread> threads(std::thread::hardware_concurrency());
	const int grainSize = numFrames / threads.size() + 1;
	for (int t = 0; t < threads.size(); ++t) {
		threads[t] = std::thread([=, &RMSDs, &xyz, &rmsdsDone] {
			const int maxFr =
			        std::min((t + 1) * grainSize, numFrames);
			for (int fr = t * grainSize; fr < maxFr; ++fr) {
				for (int j = fr; j < numFrames; ++j) {
					const float *xyz_fr =
					        xyz.data() + 3 * fr * nAtoms;
					const float *xyz_j =
					        xyz.data() + 3 * j * nAtoms;
					RMSDs(fr, j) = msd_atom_major(
					        nAtoms, nAtoms, xyz_fr, xyz_j,
					        traces[fr], traces[j], 0,
					        nullptr);
					RMSDs(j, fr) = RMSDs(fr, j);
				}
				rmsdsDone += numFrames - fr;
				percDone = rmsdsDone * 100 / numRmsds;
			}
		});
	}
	for (auto &&t : threads) {
		t.join();
	}

	return RMSDs.cwiseSqrt() * 10.0f;


	/*pteros::Selection s(traj,"all");
	for(int i=0; i<numFrames; ++i) {
		s.fit_trajectory(i,i);
		for (int j=i; j<numFrames;++j) {
			RMSDs(i,j)=RMSDs(j,i)=s.rmsd(i,j)*10.0f;
		}
		rmsdsDone+=numFrames-i;
	}
	//dump_rmsds(RMSDs,"rmsds_seq.dat");
	return RMSDs;*/
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
		percDone = i * 100 / numFrames;
	}
	if (numFrames != traj.num_frames()) {
		std::cout << "total frames loaded "
		                     + std::to_string(traj.num_frames()) + "/"
		                     + std::to_string(numFrames) + "\n"
		          << std::flush;
		return System();
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
	if (numFrames == 0) {
		return;
	}

	MatrixXf RMSDs;
	showProgress("Calculating RMSD...", [&] { RMSDs = rmsds(traj); });

	if (RMSDs.hasNaN()) {
		std::cerr << "RMSDs has NAN!\n" << std::flush;
		return;
	}

	// Cleanup Efficiencies
	std::vector<std::string> pairNames = evalNames;
	const double maxNanFraction = 0.2;
	const int maxNan = int(maxNanFraction * effs.rows());
	std::vector<unsigned> keepIdxs;
	for (unsigned i = 0; i < effs.cols(); ++i) {
		int numNan = effs.array().col(i).isNaN().cast<int>().sum();
		if (numNan < maxNan) {
			keepIdxs.push_back(i);
		} else {
			pairNames[i].erase();
		}
	}
	Eigen::MatrixXf E = sliceCols(effs, keepIdxs);
	auto isEmptyStr = [](const std::string &s) { return s.empty(); };
	auto it = remove_if(pairNames.begin(), pairNames.end(), isEmptyStr);
	pairNames.erase(it, pairNames.end());

	// replace NaNs with Efficiencies from similar structures
	const float maxFloat = std::numeric_limits<float>::max();
	for (unsigned col = 0; col < E.cols(); ++col) {
		const Eigen::VectorXf filter =
		        E.col(col).array().isNaN().matrix().cast<float>()
		        * maxFloat;
		for (unsigned row = 0; row < E.rows(); ++row) {
			if (not std::isnan(E(row, col))) {
				continue;
			}
			auto fRMSDS = RMSDs.col(row).cwiseMax(filter);
			unsigned similarConf;
			fRMSDS.minCoeff(&similarConf);
			E(row, col) = E(similarConf, col);
		}
	}

	const int maxPairs = std::min(int(E.cols()), numPairsMax);
	std::vector<unsigned> pairList;
	showProgress("Greedy selection...", [&]() {
		pairList =
		        greedySelection(err, E, RMSDs, maxPairs, numFitParams);
	});

	std::string report = "#\tPair_added\t<<RMSD>>/A\n";
	std::vector<unsigned> tmpPairs;
	for (int i = 0; i < pairList.size(); ++i) {
		tmpPairs.push_back(pairList[i]);
		auto chi2 = chiSquared(sliceCols(E, tmpPairs), err);
		float rmsdAve = rmsdMeanMean(RMSDs, chi2, i + 1, 0.99f);
		report += std::to_string(i + 1) + "\t" + pairNames[pairList[i]]
		          + "\t" + std::to_string(rmsdAve) + "\n";
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
	percDone = 0;
	QProgressDialog dialog(title, QString(), 0, 100, this);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.setMinimumDuration(0);
	auto &percDoneRef = percDone;
	auto f = std::async(std::launch::async, func);
	std::future_status status = f.wait_for(std::chrono::milliseconds(0));
	while (status != std::future_status::ready) {
		// setValue() calls processEvents() only if percDone has changed
		dialog.setValue(percDone);
		QApplication::processEvents(); // makes GUI more responsive
		status = f.wait_for(std::chrono::milliseconds(20));
	}
	f.get();
	dialog.setValue(100);
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
		percDone = 100 * pairsDone / maxPairs;
	}
	return selPairs;
}
