#include "GetInformativePairsDialog.h"
#include "ui_GetInformativePairsDialog.h"
#include "best_dist.h"
#include <pteros/pteros.h>
#include <QProgressDialog>
#include <future>

GetInformativePairsDialog::GetInformativePairsDialog(
		QWidget *parent, const std::vector<FrameDescriptor>& frames,
		const Eigen::MatrixXf& effs,
		const std::vector<std::string>& evalNames) :
	QDialog(parent),frames(frames),effs(effs),evalNames(evalNames),
	ui(new Ui::GetInformativePairsDialog)
{
	ui->setupUi(this);
}

GetInformativePairsDialog::~GetInformativePairsDialog()
{
	delete ui;
}

void dump_rmsds(const Eigen::MatrixXf& m, const std::string& fname)
{
	std::ofstream outfile;
	outfile.open(fname, std::ifstream::out);
	if(!outfile.is_open())
	{
		return;
	}
	for(int i=0; i<m.rows(); ++i){
		outfile<<m(i,0)*0.1;
		for (int j=1; j<m.cols(); ++j) {
			outfile<<"\t"<<m(i,j)*0.1;
		}
		outfile<<"\n";
	}
}

void GetInformativePairsDialog::accept()
{
	using Eigen::MatrixXf;
	using namespace pteros;
	const std::string sel="name CA";
	const double err=0.058;
	const int numSelectionMax=10;

	FRETEfficiencies Eall(err,effs.rows());
	Eall.setFromEffMatrix(effs,evalNames);

	MatrixXf RMSDs(frames.size(),frames.size());

	QProgressDialog loadProgress("Loading structures...",QString(),0,
				     frames.size(),this);
	loadProgress.setWindowModality(Qt::WindowModal);
	loadProgress.setValue(0);
	System traj(frames[0].topologyFileName());
	traj.keep(sel);
	for(int i=1; i<frames.size(); ++i) {
		const FrameDescriptor& fr=frames[i];
		System system(fr.topologyFileName());
		system.keep(sel);
		traj.frame_append(system.Frame_data(0));
		loadProgress.setValue(i);
	}
	loadProgress.setValue(frames.size());

	const size_t numFrames=frames.size();
	QProgressDialog rmsdProgress("Calculating RMSD...",QString(),0,
				     numFrames,this);
	rmsdProgress.setWindowModality(Qt::WindowModal);
	for(int i=0; i<numFrames; ++i) {
		rmsdProgress.setValue(i);
		Selection(traj,"all").fit_trajectory(i);
		for (int j=i; j<numFrames;++j) {
			Selection s(traj,"all");
			RMSDs(i,j)=RMSDs(j,i)=s.rmsd(i,j)*10.0f;
		}
	}
	rmsdProgress.setValue(numFrames);

	const int maxPairs=std::min(Eall.distanceCount(),ui->maxPairs->value());
	QProgressDialog selectionProgress("Greedy selection...",QString(),0,
					  maxPairs,this);
	selectionProgress.setWindowModality(Qt::WindowModal);
	selectionProgress.setMinimumDuration(0);
	std::future<void> selection(std::async(std::launch::async,[&,this]{
		greedySelection(err,Eall,RMSDs,maxPairs);
	}));
	std::future_status status;
	do {
		QApplication::processEvents();
		selectionProgress.setValue(pairsDone-1);
		status = selection.wait_for(std::chrono::milliseconds(20));
	} while (status != std::future_status::ready);
	selectionProgress.setValue(maxPairs);
	QDialog::accept();
}

void GetInformativePairsDialog::greedySelection(const float err,
						const FRETEfficiencies& Eall,
						const Eigen::MatrixXf& RMSDs,
						const int maxPairs) const
{
	std::stringstream ss;
	ss.str(std::string());

	FRETEfficiencies E(err,Eall.conformerCount());

	std::cout<<"#\tAdded distance\t<<RMSD>>\n";
	for (pairsDone=0; pairsDone<maxPairs; ++pairsDone) {
		int b=E.bestDistance(RMSDs,Eall);
		ss<<pairsDone+1<<'\t'<<Eall.name(b);
		E.addDistance(Eall,b);
		ss<<"\t"<<std::setprecision(2)<<E.rmsdAve(RMSDs)<<std::endl;
		std::cout<<ss.str();
		ss.str(std::string());
	}
}
