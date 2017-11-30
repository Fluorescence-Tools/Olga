#include "GetInformativePairsDialog.h"
#include "ui_GetInformativePairsDialog.h"
#include "best_dist.h"
#include "theobald_rmsd.h"
#include "center.h"
#include <pteros/pteros.h>
#include <QProgressDialog>
#include <QFileDialog>
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
std::string GetInformativePairsDialog::
list2str(const GetInformativePairsDialog::pair_list_type &list)
{
	std::stringstream ss;
	ss<<"#\tAdded distance\t<<RMSD>>\n";
	std::ofstream outfile;
	ss<<std::setprecision(3);
	for (int i=0; i<list.size(); ++i) {
		ss<<i<<"\t"<<list[i].first<<"\t"<<list[i].second<<"\n";
	}

	return ss.str();
}

std::vector<float> sys2xyz(const pteros::System& s)
{
	std::vector<float> vec;
	vec.reserve(s.num_atoms()*s.num_frames()*3);
	for(int fr=0; fr<s.num_frames(); ++fr) {
		const float* beg=s.Frame_data(fr).coord.data()->data();
		vec.insert(vec.end(),beg,beg+3*s.num_atoms());
		/*
		for(int at=0; at<s.num_atoms(); ++at) {
			const float* beg=s.Frame_data(fr).coord.at(at).data();
			vec.insert(vec.end(),beg,beg+3);
		}*/
	}
	return vec;
}

/*Eigen::MatrixXf rmsds(const pteros::System& s)
{
	const int numFrames=s.num_frames();
	const int nAtoms=s.num_atoms();
	std::vector<float> xyz=sys2xyz(s);
	std::vector<float> traces(numFrames);
	inplace_center_and_trace_atom_major(xyz.data(),traces.data(),
					    numFrames, nAtoms);
	Eigen::MatrixXf RMSDs(numFrames,numFrames);

	for (int fr=0; fr<numFrames; ++fr) {
		for (int j=fr; j<numFrames;++j) {
			float* coords_fr=xyz.data()+3*fr*nAtoms;
			float* coords_j=xyz.data()+3*j*nAtoms;
			RMSDs(fr,j)=msd_atom_major(nAtoms,nAtoms,
						   coords_fr,coords_j,
						   traces[fr], traces[j],
						   0,nullptr);
			RMSDs(j,fr)=RMSDs(fr,j);
		}
	}
	RMSDs=RMSDs.cwiseSqrt()*10.0f;
	//dump_rmsds(RMSDs,"rmsds_mdtraj.dat");
	return RMSDs;
}*/

Eigen::MatrixXf GetInformativePairsDialog::rmsds(const pteros::System &traj) const
{
	const int numFrames=traj.num_frames();
	const int nAtoms=traj.num_atoms();
	std::vector<float> xyz=sys2xyz(traj);
	std::vector<float> traces(numFrames);
	inplace_center_and_trace_atom_major(xyz.data(),traces.data(),
					    numFrames, nAtoms);
	Eigen::MatrixXf RMSDs(numFrames,numFrames);

	rmsdsDone=0;

	std::vector<std::thread> threads(std::thread::hardware_concurrency());
	const int grainSize=numFrames/threads.size()+1;
	for(int t=0; t<threads.size(); ++t) {
		threads[t]=std::thread([=,&RMSDs,&xyz] {
			const int maxFr=std::min((t+1)*grainSize,numFrames);
			for (int fr=t*grainSize; fr<maxFr; ++fr) {
				for (int j=fr; j<numFrames;++j) {
					const float* xyz_fr=xyz.data()+3*fr*nAtoms;
					const float* xyz_j=xyz.data()+3*j*nAtoms;
					RMSDs(fr,j)=msd_atom_major(nAtoms,nAtoms,
								   xyz_fr,xyz_j,
								   traces[fr],
								   traces[j],
								   0,nullptr);
					RMSDs(j,fr)=RMSDs(fr,j);
				}
				rmsdsDone+=numFrames-fr;
			}
		});
	}
	for(auto&& t : threads) {
		t.join();
	}

	return RMSDs.cwiseSqrt()*10.0f;


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

void GetInformativePairsDialog::accept()
{
	using std::future;
	using std::string;
	using std::map;
	using Eigen::MatrixXf;
	using std::async;
	using namespace pteros;
	const std::string sel=ui->selectionEdit->text().toStdString();
	const double err=ui->errorSpinBox->value();
	const int numPairsMax=ui->maxPairs->value();
	const int numFitParams=ui->numFitParams->value();
	const size_t numFrames=frames.size();

	QProgressDialog setEffProgress("Initializing E matrix...",QString(),0,
				       100,this);
	setEffProgress.setWindowModality(Qt::WindowModal);
	setEffProgress.setMinimumDuration(0);
	setEffProgress.setValue(0);
	std::future<FRETEfficiencies> fEall(async(std::launch::async,[&,this] {
		FRETEfficiencies Eall(err,effs.rows());

		std::thread th=std::thread([&]{
			while(percDone<100) {
				percDone.store(Eall.percDone-1);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		});

		Eall.setFromEffMatrix(effs,evalNames);
		//TODO:this is a hack
		Eall.percDone=percDone=101;
		th.join();
		return Eall;
	}));
	std::future_status status;
	do {
		QApplication::processEvents();
		setEffProgress.setValue(percDone);
		status = fEall.wait_for(std::chrono::milliseconds(20));
	} while (status != std::future_status::ready);
	const FRETEfficiencies& Eall=fEall.get();
	setEffProgress.setValue(100);

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
		if(traj.num_atoms()==system.num_atoms()) {
			traj.frame_append(system.Frame_data(0));
		} else {
			std::cerr<<"ERROR! frame number does not match "
				   +fr.topologyFileName()
				   +" atoms: "+std::to_string(system.num_atoms())
				   +"/"+std::to_string(traj.num_atoms())+"\n"<<std::flush;
		}
		loadProgress.setValue(i);
	}
	loadProgress.setValue(numFrames);
	if(numFrames!=traj.num_frames()) {
		std::cout<<"total frames loaded "+std::to_string(traj.num_frames())
			   +"/"+std::to_string(numFrames)+"\n"<<std::flush;
		return;
	}

	const size_t numRmsds=numFrames*numFrames/2;
	QProgressDialog rmsdProgress("Calculating RMSD...",QString(),0,
				     numRmsds,this);
	rmsdProgress.setWindowModality(Qt::WindowModal);
	rmsdProgress.setMinimumDuration(0);
	//	auto start = std::chrono::system_clock::now();
	std::future<MatrixXf> fRmsds(async(std::launch::async,[&,this]{
		return rmsds(traj);
	}));
	do {
		QApplication::processEvents();
		rmsdProgress.setValue(rmsdsDone-1);
		status = fRmsds.wait_for(std::chrono::milliseconds(20));
	} while (status != std::future_status::ready);
	const MatrixXf& RMSDs=fRmsds.get();
	if(RMSDs.hasNaN()) {
		std::cerr<<"RMSDs has NAN!\n"<<std::flush;
		return;
	}
	//dump_rmsds(RMSDs,"state_rmsds.dat");
	//	std::chrono::duration<double> diff = std::chrono::system_clock::now()-start;
	//	std::cout<<"\n\nrmsds/s: "+std::to_string(numRmsds/diff.count())<<std::endl;
	rmsdProgress.setValue(numRmsds);

	const int maxPairs=std::min(Eall.distanceCount(),numPairsMax);
	QProgressDialog selectionProgress("Greedy selection...",QString(),0,
					  100,this);
	selectionProgress.setWindowModality(Qt::WindowModal);
	selectionProgress.setMinimumDuration(0);
	future<pair_list_type> selection(async(std::launch::async,[&,this]{
		return greedySelection(err,Eall,RMSDs,maxPairs,numFitParams);
	}));
	do {
		QApplication::processEvents();
		selectionProgress.setValue(percDone);
		status = selection.wait_for(std::chrono::milliseconds(20));
	} while (status != std::future_status::ready);
	pair_list_type distList=selection.get();
	selectionProgress.setValue(100);

	std::string selReport=list2str(distList);
	auto report="Greedy selection:\n"+selReport;
	if(false) {
		distList=miSelection(err,Eall,RMSDs,maxPairs);
		std::string miReport=list2str(distList);
		report+="\nMutual Information selection:\n"+miReport;
	}

	std::string fname=ui->fileEdit->text().toStdString();
	if(fname.empty()) {
		std::cout<<report<<std::flush;
	} else {
		std::ofstream outfile(fname, std::ifstream::out);
		if(!outfile.is_open())
		{
			std::cerr<<"Warning! could not open file for saving: "
				   +fname+"\n"<<std::flush;
			return;
		}
		outfile<<report;
	}

	QDialog::accept();
}


void GetInformativePairsDialog::setFileName()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("report file"),
							"", tr("Text file (*.txt);;Any file (*)"));
	ui->fileEdit->setText(fileName);
}

GetInformativePairsDialog::pair_list_type GetInformativePairsDialog::
greedySelection(const float err, const FRETEfficiencies& Eall,
		const Eigen::MatrixXf& RMSDs, const int maxPairs,
		const int fitParams) const
{
	FRETEfficiencies E(err,Eall.conformerCount(),fitParams);
	pair_list_type result;

	std::atomic<int> pairsDone{0};

	std::thread th=std::thread([&]{
		while(pairsDone<maxPairs) {
			percDone=100*pairsDone/maxPairs+E.percDone/maxPairs;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});

	for (pairsDone=0; pairsDone<maxPairs; ++pairsDone) {
		int b=E.bestDistance(RMSDs,Eall);
		E.addDistance(Eall,b);
		float aveRmsd=E.rmsdAve(RMSDs);
		result.emplace_back(Eall.name(b),aveRmsd);
	}
	th.join();
	return result;
}

GetInformativePairsDialog::pair_list_type GetInformativePairsDialog::
miSelection(const float err, const FRETEfficiencies &Eall,
	    const Eigen::MatrixXf &RMSDs, const int maxPairs) const
{
	FRETEfficiencies E(err,Eall.conformerCount());
	pair_list_type result;

	int b=E.bestDistance(RMSDs,Eall);
	E.addDistance(Eall,b);
	float aveRmsd=E.rmsdAve(RMSDs);
	result.emplace_back(Eall.name(b),aveRmsd);

	for (int i=1; i<maxPairs; ++i) {
		b=E.bestDistanceMI(Eall);
		E.addDistance(Eall,b);
		float aveRmsd=E.rmsdAve(RMSDs);
		result.emplace_back(Eall.name(b),aveRmsd);
	}
	return result;
}
