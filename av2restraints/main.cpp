#include "pteros/pteros.h"
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <set>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QFile>

#include "TaskStorage.h"
#include "AbstractEvaluator.h"
#include "AV/PositionSimulationResult.h"
#include "CalcResult.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"
std::ostream& operator<<(std::ostream& os, const EvalId& id)
{
	os << static_cast<int>(id);
	return os;
}
struct Restrt {
	std::vector<Eigen::Vector3d> coords;
	std::vector<Eigen::Vector3d> velocities;
	std::string header, footer;
	bool load(const std::string fileName) {
		std::ifstream f;
		f.open(fileName,std::ios::binary);
		int fsize=-f.tellg();
		f.seekg(0,std::ios::end);
		fsize+=f.tellg();
		f.close();
		f.open(fileName);
		if(!f.is_open()) {
			return false;
		}
		std::getline(f,header);
		std::string tmp;
		std::getline(f,tmp);
		header+="\n"+tmp;
		//trim
		tmp.erase(tmp.begin(), std::find_if(tmp.begin(), tmp.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		int natoms=std::stoi(tmp.substr(0,tmp.find(" ")));
		coords.clear();
		coords.reserve(natoms);
		velocities.clear();
		velocities.reserve(natoms);
		for(int i=0; i<natoms; i++) {
			Eigen::Vector3d v;
			f>>v[0]>>v[1]>>v[2];
			coords.push_back(v);
		}

		if(fsize-2.0*(size_t(f.tellg())-header.size())>0) {
			for(int i=0; i<natoms; i++) {
				Eigen::Vector3d v;
				f>>v[0]>>v[1]>>v[2];
				velocities.push_back(v);
			}
		} else {
			std::cout<<"WARNING: velocities are not read from restart file"<<std::endl;
			std::cout<<"fsize="<<fsize<<", tellg="<<f.tellg()<<", header="<<header.size()<<std::endl;
			velocities.resize(natoms);
		}
		std::getline(f,footer);
		std::getline(f,tmp);
		footer+=tmp+"\n";
		return true;
	}
	void save(const std::string fileName) {
		std::ofstream f;
		f.open(fileName);
		f<<header<<"\n";
		using std::setw;
		using std::setprecision;
		using std::setiosflags;
		using std::ios;
		for(const auto& vec:{coords,velocities}) {
			for(int i=0; i<coords.size(); ++i) {
				const auto& v=vec[i];
				for(int k:{0,1,2}) {
					f<<setiosflags(ios::fixed)
					<<setw(12)<<setprecision(7)<<v[k];
				}
				if(i%2==1) {
					f<<"\n";
				}
			}
			if(coords.size()%2==1) {
				f<<"\n";
			}
		}

		f<<footer;
	}
	void setCoords(int atomId, const Eigen::Vector3d& c) {
		coords[atomId-1]=c;
	}

	int atomId(const Eigen::Vector3d& pos) const
	{
		const double epsilon=0.05;
		for(int i=0; i<coords.size(); ++i) {
			const Eigen::Vector3d& v=coords[i];
			double dist=(pos-v).norm();
			if( dist<epsilon) {
				return i+1;
			}
		}
		return -1;
	}
	void appendAtom(const Eigen::Vector3d c) {
		coords.push_back(c);
		velocities.push_back(Eigen::Vector3d(0.0f,0.0f,0.0f));
	}
};
struct NmrRestraint;
std::ostream& operator<<(std::ostream& os, const NmrRestraint& r);
struct NmrRestraint {
	int _iat1, _iat2, _nstep1=1, _nstep2;
	double r1, r2,r3,r4,rk2,rk3;
	double r1a,r2a,r3a,r4a,rk2a,rk3a;
	std::string note;
	NmrRestraint(const double Fmax1, const double Fmax2,
		     const int iat1, int iat2, int nstep2,
		     const Distance& dist)
	{
		using std::to_string;
		_iat1=iat1; _iat2=iat2; _nstep1=1; _nstep2=nstep2;
		note=dist.position1()+" ("+to_string(_iat1)+") <--> "
		     +dist.position2()+" ("+to_string(_iat2)+") ";
		r2=r3=dist.RmpFromModelDistance();
		r1=r2-dist.errNeg();
		r4=r3+dist.errPos();
		r1a=r1; r2a=r2; r3a=r3; r4a=r4;

		//force constants
		rk2=0.5*Fmax1/dist.errNeg();
		rk3=0.5*Fmax1/dist.errPos();
		rk2a=0.5*Fmax2/dist.errNeg();
		rk3a=0.5*Fmax2/dist.errPos();
	}
	double force(double currentDistance, bool atstep1) const
	{
		double k2=atstep1?rk2:rk2a;
		double k3=atstep1?rk3:rk3a;
		if(currentDistance<r1) {
			return -2.0*k2*(r1-r2);
		} else if (currentDistance<r2) {
			return -2.0*k2*(currentDistance-r2);
		} else if(currentDistance<r3) {
			return 0.0;
		} else if(currentDistance<r4) {
			return -2.0*k3*(currentDistance-r3);
		} else {
			return -2.0*k3*(r4-r3);
		}
	}

	std::string toString() const {
		std::string str="#"+note+"\n";
		using std::to_string;
		str+="&rst iat = "+to_string(_iat1)+", "+to_string(_iat2)+
		     ", nstep1 = "+to_string(_nstep1)+
		     ", nstep2 = "+to_string(_nstep2)+
		     ", ifvari = 1"+
		     ", r1 = "+to_string(r1)+", r2 = "+to_string(r2)+
		     ", r3 = "+to_string(r3)+", r4 = "+to_string(r4)+
		     ", rk2 = "+to_string(rk2)+", rk3 = "+to_string(rk3)+
		     ", r1a = "+to_string(r1a)+", r2a = "+to_string(r2a)+
		     ", r3a = "+to_string(r3a)+", r4a = "+to_string(r4a)+
		     ", rk2a = "+to_string(rk2a)+", rk3a = "+to_string(rk3a)+
		     ",\n/\n";
		return str;
	}
	static void save(const std::vector<NmrRestraint>& vec,
			 const std::string& fileName)
	{
		std::ofstream f;
		f.open(fileName);
		for(const auto& r:vec) {
			f<<r.toString();
		}
	}
	static std::map<int,Eigen::Vector3d>
	totalForce(const std::vector<NmrRestraint>& vec,
		   const std::map<int,Eigen::Vector3d>& duIdPos, bool atstep1)
	{
		std::map<int,Eigen::Vector3d> totalForce;
		for(const auto& pair:duIdPos) {
			totalForce[pair.first]={0.0,0.0,0.0};
		}
		for(const NmrRestraint& rst:vec) {
			Eigen::Vector3d r=duIdPos.at(rst._iat1)-duIdPos.at(rst._iat2);
			const auto& force1=rst.force(r.norm(),atstep1)*r.normalized();
			totalForce[rst._iat1]+=force1;
			totalForce[rst._iat2]-=force1;
		}
		return totalForce;
	}
	static std::vector<NmrRestraint>
	capForce(const std::vector<NmrRestraint>& vec,
		 const std::map<int,Eigen::Vector3d>& duIdPos)
	{
		std::vector<NmrRestraint> caped=vec;
		using std::pair;

		bool atstep1=true;
		const double inf=std::numeric_limits<double>::infinity();
		double Fcap=vec.front().force(inf,atstep1);
		Fcap=std::fabs(Fcap);

		auto totalF=totalForce(caped,duIdPos,atstep1);

		auto pr = std::max_element(totalF.begin(), totalF.end(),
					   [](const pair<int,Eigen::Vector3d>& p1,
					   const pair<int,Eigen::Vector3d>& p2) {
			return p1.second.norm() < p2.second.norm(); });

		while(pr->second.norm() > Fcap) {
			for(auto& p:caped) {
				if(p._iat1==pr->first || p._iat2==pr->first) {
					p.rk2*=Fcap/pr->second.norm();
					p.rk3*=Fcap/pr->second.norm();
				}
			}
			totalF=totalForce(caped,duIdPos,atstep1);
			pr = std::max_element(totalF.begin(), totalF.end(),
					      [](const pair<int,Eigen::Vector3d>& p1,
					      const pair<int,Eigen::Vector3d>& p2) {
				return p1.second.norm() < p2.second.norm(); });
		}

		std::cout<<"Total Force per DU (nstep1) =\n";print(totalF);std::cout<<std::endl;

		atstep1=false;
		Fcap=vec.front().force(inf,atstep1);
		Fcap=std::fabs(Fcap);

		totalF=totalForce(caped,duIdPos,atstep1);
		pr = std::max_element(totalF.begin(), totalF.end(),
					   [](const pair<int,Eigen::Vector3d>& p1,
					   const pair<int,Eigen::Vector3d>& p2) {
			return p1.second.norm() < p2.second.norm(); });

		while(pr->second.norm() > Fcap) {
			for(auto& p:caped) {
				if(p._iat1==pr->first || p._iat2==pr->first) {
					p.rk2a*=Fcap/pr->second.norm();
					p.rk3a*=Fcap/pr->second.norm();
				}
			}
			totalF=totalForce(caped,duIdPos,atstep1);
			pr = std::max_element(totalF.begin(), totalF.end(),
					      [](const pair<int,Eigen::Vector3d>& p1,
					      const pair<int,Eigen::Vector3d>& p2) {
				return p1.second.norm() < p2.second.norm(); });
		}
		std::cout<<"Total Force per DU (nstep2) =\n";print(totalF);std::cout<<std::endl;

		return caped;
	}
	static void print(const std::map<int,Eigen::Vector3d>& map)
	{
		for(const auto& pair:map)
		{
			std::cout<<pair.first<<": |"<<pair.second.transpose()<<"|*69.4786 =\t"<<pair.second.norm()*69.4786<<std::endl;
		}
	}
	static void print(const std::vector<NmrRestraint>& vec) {
		for (const auto& r:vec) {
			std::cout<<r<<std::endl;
		}
	}
};
std::ostream& operator<<(std::ostream& os, const NmrRestraint& r)
{
    os << r.toString();
    return os;
}

const double errAnchor=2.0; //Angstrom
const double FmaxMultAnchor=2.0; //2 fold
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	QCoreApplication::setApplicationName("av2restraints");
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOptions({
				  {{"p", "pdb"},
				   "PDB file to use for AV simulations","file"},
				  {"savepdb", "save PDB file with updated DUs","file"},
				  {{"j","json"},
				   "setting file describing labelig positions and distances", "file"},
				  {"ir","Reference restart file, corresponding to the specified PDB", "file"},
				  {"o","Name for the generated restraints file", "file"},
				  {"or","Name for the generated restart file", "file"},
				  {{"n","nstep2"},"Number of steps in the run", "int"},
				  {"f1","Max force at the beginning of the run [pN]", "float"},
				  {"f2","Max force at the end of the run [pN]", "float"},
				  {"nocap","Do not limit the per-dummy total Force,only per-distance"}
			  });
	parser.process(a);
	QString settingsFileName=parser.value("j");
	QString pdbFileName=parser.value("p");
	QString restartInFileName=parser.value("ir");
	QString restartOutFileName=parser.value("or");
	QString restraintsFileName=parser.value("o");
	const int nstep2=parser.value("nstep2").toInt();
	//convert from pN to kcal/mol Angstrom
	const double f1=parser.value("f1").toDouble() / 69.4786;
	const double f2=parser.value("f2").toDouble() / 69.4786;
	const bool nocap=parser.isSet("nocap");

	//check inputs
	if(nstep2<=0) {
		std::cout<<"nstep2 must be a positive integer"<<std::endl;
		return 1;
	}
	if(f1 <0.0 || f2<0.0) {
		std::cout<<"f1 and f2 must be non-negative"<<std::endl;
		return 2;
	}

	QFile settingsFile(settingsFileName);
	if (!settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cout<<"Unable to open file "<<settingsFileName.toStdString()
			<<settingsFile.errorString().toStdString()<<std::endl;
		return 3;
	}
	pteros::System sys;
	try {
		sys.load(pdbFileName.toStdString());
	} catch (...) {
		std::cout<<"Unable to open file "<<pdbFileName.toStdString()<<std::endl;
		return 4;
	}
	Restrt restrt;
	if(!restrt.load(restartInFileName.toStdString())) {
		std::cout<<"Unable to load file "<<restartInFileName.toStdString()<<std::endl;
		return 5;
	}


	using std::ios;
	using std::setiosflags;
	using std::setprecision;
	using std::setw;

	pteros::Selection selDU=sys.select(std::string("resname DU"));

	if(restrt.coords.size() != sys.num_atoms()) {
		std::cerr<<"FATAL error: number of atoms in pdb and restrt do not match"<<std::endl;
		return 6;
	}

	TaskStorage storage;


	QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
	storage.loadEvaluators(doc.toVariant().toMap());
	using std::vector;

	//get all the AV and Distance Evaluator ids.
	vector<EvalId> avs, distances;
	for(const auto& pair:storage.evals()) {
		if(storage.isStub(pair.first)) {
			continue;
		}
		if(pair.second->className()=="Distances") {
			distances.push_back(pair.first);
		}
		if(pair.second->className()=="Positions") {
			avs.push_back(pair.first);
		}
	}
	const int numDUAtoms=selDU.size();
	std::cout<<numDUAtoms<<" DU atoms, "<<avs.size()<<" positions and "<<distances.size()<<" distances"<<std::endl;

	if(numDUAtoms!=avs.size() && numDUAtoms!=0) {
		std::cerr<<"FATAL error: Could not mathch Labelling Positions to Pseudoatoms!"<<std::endl;
		return 1;
	}

	//insert Pseudoatoms
	if(numDUAtoms == 0) {
		pteros::Atom atom;
		atom.name="DU";
		atom.resname="DU";
		atom.mass=100.0f;
		atom.charge=0.0f;
		auto resids=sys.select(std::string("all")).get_unique_resid();
		auto it=std::max_element(resids.begin(),resids.end());
		atom.resid=*it+1;
		auto chains=sys.select(std::string("all")).get_unique_chain ();
		atom.chain=*std::max_element(chains.begin(),chains.end())+1;
		atom.chain=atom.chain=='!'?'B':atom.chain;
		for(int i=0; i<avs.size(); i++) {
			auto c=Eigen::Vector3f(i,3.14f,3.14f);
			restrt.appendAtom(c.cast<double>());
			pteros::System tmpSys;
			tmpSys.append(atom,c*0.1f);
			sys.append(tmpSys);
			++atom.resid;
			++atom.chain;
		}
	}

	selDU.update();//apply()?
	auto duXYZf=selDU.get_xyz();
	auto duXYZd=duXYZf.cast<double>()*10.0;
	std::cout<<"DU coordinates read:\n"<<duXYZd<<"\n";

	FrameDescriptor frame(pdbFileName.toStdString(),pdbFileName.toStdString());
	storage.evaluate(frame,avs);
	std::cout<<"Evaluation started, waiting..."<<std::endl;
	while(!storage.ready()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::cout<<"waiting..."<<std::endl;
	}

	using std::pair;
	using std::map;
	map<EvalId,pair<EvalId,EvalId>> dist2lps; //match distances to AVs
	map<EvalId,std::string> lpNames;
	map<EvalId,Distance> distInfo;
	for(const auto& dist:distances) {
		EvalId lp1=storage.eval(dist).settingValue(1).value<EvalId>();
		EvalId lp2=storage.eval(dist).settingValue(2).value<EvalId>();
		lpNames[lp1]=storage.eval(lp1).name();
		lpNames[lp2]=storage.eval(lp2).name();
		dist2lps.insert({dist,std::make_pair(lp1,lp2)});
		distInfo[dist]=static_cast<const EvaluatorDistance&>(storage.eval(dist)).distance();
		std::cout<<dist<<" "<<lp1<<" "<<lp2<<std::endl;
	}

	map<EvalId,int> av2atomId; //match AV_MPs to atomIDs
	//match  AV_MPs to atom position
	std::map<int,Eigen::Vector3d> atomid2Pos;
	for(const auto& avId:avs) {
		TaskStorage::Result res=storage.getResult(frame,avId);
		const int curXYZind=av2atomId.size();
		Eigen::Vector3d oldcoords=duXYZd.col(curXYZind);
		const int atomId=restrt.atomId(oldcoords);
		av2atomId[avId]=atomId;
		atomid2Pos[atomId]=oldcoords;

		if(!res) {continue;}//somehow av calculation has failed, stay with old coords
		std::shared_ptr<CalcResult<PositionSimulationResult>> calcpos;
		calcpos=std::static_pointer_cast<CalcResult<PositionSimulationResult>>(res);
		PositionSimulationResult pos=calcpos->get();
		if(pos.empty()) {continue;} //stay with old coords
		Eigen::Vector3d newcoords=pos.meanPosition().cast<double>();
		atomid2Pos[atomId]=newcoords;
		//restrt.replaceCoords(oldcoords,newcoords);
		restrt.setCoords(atomId,newcoords);
		duXYZf.col(curXYZind)=newcoords.cast<float>()*0.1f;
	}
	std::cout<<std::endl;

	restrt.save(restartOutFileName.toStdString());
	if(parser.isSet("savepdb")) {
		selDU.set_xyz(duXYZf);
		sys.select(std::string("all")).write(parser.value("savepdb").toStdString());
	}

	std::vector<NmrRestraint> nmrVec;
	//add restraints between pseudoatoms
	for(const auto& distId:distances) {
		EvalId lp1=dist2lps.at(distId).first;
		EvalId lp2=dist2lps.at(distId).second;
		int iat1=av2atomId[lp1];
		int iat2=av2atomId[lp2];
		const Distance& opt=distInfo.at(distId);

		nmrVec.emplace_back(f1,f2,iat1,iat2,nstep2,opt);
	}
	if(!nocap) {
		nmrVec=NmrRestraint::capForce(nmrVec,atomid2Pos);
	}


	//add restraints from pseudoatoms to the molecule
	for(const auto& avId:avs) {
		using std::to_string;
		std::string anchStr;
		anchStr=static_cast<const EvaluatorPositionSimulation&>(storage.eval(avId)).anchorAtoms();
		auto sel=sys.select(anchStr);
		auto anchXYZf=sel.get_xyz();
		auto anchXYZd=anchXYZf.cast<double>()*10.0;
		vector<int> resids=sel.get_resid();
		vector<std::string> atNames=sel.get_name();
		for(int c=0; c<anchXYZd.cols(); ++c) {
			Eigen::Vector3d anchpos=anchXYZd.col(c);
			int iat1=av2atomId.at(avId);
			int iat2=restrt.atomId(anchpos);
			Distance d;
			d.setDistance((atomid2Pos.at(iat1)-anchpos).norm());
			d.setType("Rmp");
			d.setErrNeg(errAnchor);
			d.setErrPos(errAnchor);
			d.setPosition1(lpNames.at(avId));
			d.setPosition2(to_string(resids[c])+"@"+atNames[c]);
			nmrVec.emplace_back(f1*FmaxMultAnchor,f2*FmaxMultAnchor,
					    iat1,iat2,nstep2,d);
		}
	}
	NmrRestraint::save(nmrVec,restraintsFileName.toStdString());
	return 0;
}
