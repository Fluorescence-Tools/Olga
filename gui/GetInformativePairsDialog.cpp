#include "GetInformativePairsDialog.h"
#include "ui_GetInformativePairsDialog.h"
#include "best_dist.h"
#include <pteros/pteros.h>
#include <QProgressDialog>

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

	const float err=0.058;
	const std::string sel="name CA";

	FRETEfficiencies Eall(err,effs.rows());
	Eall.setFromEffMatrix(effs,evalNames);
	Eall.dumpDistances("distances.ha4");

	MatrixXf RMSDs(frames.size(),frames.size());

	QProgressDialog loadProgress("Loading structures...",QString(),0,
				 frames.size(),this);
	loadProgress.setWindowModality(Qt::WindowModal);
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
		Selection(traj,"all").fit_trajectory(i);
		for (int j=i; j<numFrames;++j) {
			Selection s(traj,"all");
			RMSDs(i,j)=RMSDs(j,i)=s.rmsd(i,j)*10.0f;
		}
		rmsdProgress.setValue(i);
	}
	rmsdProgress.setValue(numFrames);
	dump_rmsds(RMSDs,"rmsds.dat");

	using std::cout;
	using std::endl;
	using std::flush;
	using std::vector;
	using std::string;
	using Eigen::MatrixXf;
	using Eigen::VectorXf;
	using Eigen::NoChange;

	std::stringstream ss;
	ss.str(std::string());

	FRETEfficiencies E(err,Eall.conformerCount());

	cout<<"#\tAdded distance\t<<RMSD>>\n";
	for (int i=0; i<std::min(Eall.distanceCount(),10); ++i) {
		int b=E.bestDistance(RMSDs,Eall);
		ss<<i+1<<'\t'<<Eall.name(b);
		E.addDistance(Eall,b);
		ss<<"\t"<<std::setprecision(2)<<E.rmsdAve(RMSDs)<<endl;
		cout<<ss.str();
		ss.str(std::string());
	}
	QDialog::accept();
}

