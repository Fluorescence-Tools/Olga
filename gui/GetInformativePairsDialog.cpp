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
	outfile<<std::setprecision(3);
	for(int i=0; i<m.rows(); ++i){
		outfile<<m(i,0)*0.1;
		for (int j=1; j<m.cols(); ++j) {
			outfile<<"\t"<<m(i,j)*0.1;
		}
		outfile<<"\n";
	}
}

Eigen::MatrixXf GetInformativePairsDialog::rmsds(const pteros::System &traj) const
{
	using pteros::Selection;
	const int numFrames=traj.num_frames();
	Eigen::MatrixXf RMSDs(numFrames,numFrames);
	rmsdsDone=0;

	std::vector<std::thread> threads(std::thread::hardware_concurrency());
	const int grainSize=numFrames/threads.size()+1;
	for(int t=0; t<threads.size(); ++t) {
		threads[t]=std::thread([&RMSDs,traj,grainSize,numFrames,t,this] {
			const pteros::System sys=traj;
			const int maxFr=std::min((t+1)*grainSize,numFrames);
			Selection s(sys,"all");
			for (int fr=t*grainSize; fr<maxFr; ++fr) {
				s.fit_trajectory(fr,fr);
				for (int j=fr; j<numFrames;++j) {
					RMSDs(fr,j)=RMSDs(j,fr)=s.rmsd(fr,j)*10.0f;
				}
				rmsdsDone+=numFrames-fr;
			}
		});
	}
	for(auto&& t : threads) {
		t.join();
	}

	return RMSDs;


	/*Selection s(traj,"all");
	for(int i=0; i<numFrames; ++i) {
		s.fit_trajectory(i,i);
		for (int j=i; j<numFrames;++j) {
			RMSDs(i,j)=RMSDs(j,i)=s.rmsd(i,j)*10.0f;
		}
		rmsdsDone+=numFrames-i;
	}
	//dump_rmsds(RMSDs,"rmsds_seq.dat");*/
	return RMSDs;
}

void GetInformativePairsDialog::accept()
{
	using Eigen::MatrixXf;
	using std::async;
	using namespace pteros;
	const std::string sel=ui->selectionEdit->text().toStdString();
	const double err=ui->errorSpinBox->value();
	const int numPairsMax=ui->maxPairs->value();
	const size_t numFrames=frames.size();

	FRETEfficiencies Eall(err,effs.rows());
	Eall.setFromEffMatrix(effs,evalNames);

	QProgressDialog loadProgress("Building trajectory...",QString(),0,
				     numFrames,this);
	loadProgress.setWindowModality(Qt::WindowModal);
	loadProgress.setValue(0);
	System traj(frames[0].topologyFileName());
	traj.keep(sel);
	for(int i=1; i<numFrames; ++i) {
		const FrameDescriptor& fr=frames[i];
		System system(fr.topologyFileName());
		system.keep(sel);
		traj.frame_append(system.Frame_data(0));
		loadProgress.setValue(i);
	}
	loadProgress.setValue(numFrames);

	const size_t numRmsds=numFrames*numFrames/2;
	QProgressDialog rmsdProgress("Calculating RMSD...",QString(),0,
				     numRmsds,this);
	rmsdProgress.setWindowModality(Qt::WindowModal);
	rmsdProgress.setMinimumDuration(0);
	//auto start = std::chrono::system_clock::now();
	std::future<MatrixXf> fRmsds(async(std::launch::async,[&,this]{
		return rmsds(traj);
	}));
	std::future_status status;
	do {
		QApplication::processEvents();
		rmsdProgress.setValue(rmsdsDone-1);
		status = fRmsds.wait_for(std::chrono::milliseconds(20));
	} while (status != std::future_status::ready);
	MatrixXf RMSDs=fRmsds.get();
	/*std::chrono::duration<double> diff = std::chrono::system_clock::now()-start;
	std::cout<<"\n\nrmsds/s: "+std::to_string(numRmsds/diff.count())<<std::endl;*/
	rmsdProgress.setValue(numRmsds);

	const int maxPairs=std::min(Eall.distanceCount(),numPairsMax);
	QProgressDialog selectionProgress("Greedy selection...",QString(),0,
					  maxPairs,this);
	selectionProgress.setWindowModality(Qt::WindowModal);
	selectionProgress.setMinimumDuration(0);
	std::future<void> selection(async(std::launch::async,[&,this]{
		greedySelection(err,Eall,RMSDs,maxPairs);
	}));
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
