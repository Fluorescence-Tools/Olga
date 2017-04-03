#ifndef BEST_DIST_H
#define BEST_DIST_H
#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <thread>
#include <map>
#include <unordered_set>

#include <chrono>

#include <Eigen/Dense>
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/exception/diagnostic_information.hpp>

std::vector<std::pair<std::string,float>> times;

void pushTime(const std::string& message) {
	static auto tprev=std::chrono::steady_clock::now();;
	auto diff = std::chrono::duration <float> (std::chrono::steady_clock::now()-tprev);
	tprev=std::chrono::steady_clock::now();
	times.emplace_back(message,diff.count());
}
void printTimes()
{
	float tot=0.0f;
	std::cout<<'\n';
	for (auto p:times) {
		std::cout<<p.first<<": "<<p.second<<"\n";
		tot+=p.second;
	}
	std::cout<<"total: "<<tot<<std::endl;
}

double chi2CdfApprox(const unsigned ndof, const double chi2)
{
	using boost::math::chi_squared;
	using boost::math::complement;
	using boost::math::cdf;
	using boost::math::quantile;

	if (ndof>3500) {
		//approximate with gaussian CDF (i.e. Erfc)
		return 0.5*boost::math::erfc((-chi2+ndof)/std::sqrt(4.0*ndof));
	}
	chi_squared dist(ndof);
	return cdf(dist,chi2);
}

class InterpChi2sf
{
private:
	const static int numSplines=16;
	const float minProb=0.0001f;
	struct Coefs
	{
		float a=0.0f,b=0.0f,c=0.0f;//y=a+b*x+c*x*x
		float inline f(const float& x) const
		{
			return a+b*x+c*x*x;
		}
	};
	float step;
	int iMax;
	std::array<Coefs,numSplines> k;

public:
	InterpChi2sf()
	{
		iMax=0;
		step=1.0f;
		for(int it=0; it<numSplines; ++it) {
			k[iMax].a=k[iMax].b=k[iMax].c=0.0f;
		}
	}

	InterpChi2sf(const unsigned ndof)
	{
		//approximate the goal function (chi_squared survival function) by the quadratic splines.
		using boost::math::chi_squared;
		using boost::math::complement;
		using boost::math::cdf;
		using boost::math::quantile;

		chi_squared dist(ndof);
		step=quantile(complement(dist,minProb))/(numSplines-1);
		iMax=numSplines-1;

		for(int it=0; it<iMax; ++it) {
			float i=it;
			float stepSq=step*step;
			float chi=step*i;
			float chiNext=chi+step;
			//float f=cdf(complement(dist,chi));
			float f=1.0-chi2CdfApprox(ndof,chi);
			//float fNext=cdf(complement(dist,chiNext));
			float fNext=1.0-chi2CdfApprox(ndof,chiNext);
			//float f90p=cdf(complement(dist,chi+step*0.9));
			float f90p=1.0-chi2CdfApprox(ndof,chi+step*0.9);
			float df=(fNext-f90p)/(step*0.1);
			k[i].a = f*(1.0f + i)*(1.0f + i) + i*(-fNext*(2.0f + i) + df*(1.0f + i)*step);
			k[i].b = (-2.0f*f*(1.0f + i) + 2.0f*fNext*(1.0f + i) - df*(1.0f + 2.0f*i)*step)/step;
			k[i].c = (f - fNext + df*step)/stepSq;
		}
		k[iMax].a=k[iMax].b=k[iMax].c=0.0f;
	}
	float inline value(const float& chi2) const
	{
		//return 1.0-boost::math::cdf(boost::math::chi_squared(ndof),chi2);
		const int i=std::min(int(chi2/step),iMax);
		//std::cout<<"i="<<i<<" chi2="<<chi2<<" iMax="<<iMax<<std::endl;
		return k[i].f(chi2);
	}
};

class FRETEfficiencies
{
	using MatrixXf=Eigen::MatrixXf;
	using VectorXf=Eigen::VectorXf;
	using string=std::string;
private:
	MatrixXf E, validity;
	MatrixXf _chi2, _nPoints;//W ;
	std::vector<string> distNames;

	const float R0=52.0f;
	const float err=0.058f;
	const int _numFitParams=1;

	mutable std::vector<InterpChi2sf> sfs;
public:
	FRETEfficiencies(const float err, const int numConf):err(err)
	{
		E.resize(numConf,0);
		validity.resize(numConf,0);
		_chi2.resize(numConf,numConf);
		_nPoints.resize(numConf,numConf);
		_chi2.setZero();
		_nPoints.setZero();
	}
	void dumpState(const string& fname) const
	{
		std::ofstream os;
		os.open(fname, std::ifstream::out);
		if(!os.is_open())
		{
			return;
		}


		for (const auto& name:distNames) {
			os<<name<<"\t";
		}
		os<<"\n\nE:\n";
		os<<E;

		os<<"\n\nvalidity:\n";
		os<<validity;

		os<<"\n\nChi2:\n";
		os<<std::setprecision(4);
		os<<std::fixed;
		MatrixXf chi2=_chi2;
		chi2.triangularView<Eigen::StrictlyLower>().setZero();
		os<<chi2;

		os<<"\n\n_nPoints:\n";
		os<<_nPoints;
	}

	const std::string& name(const int col) const
	{
		return distNames[col];
	}
	int distanceCount() const {
		return distNames.size();
	}
	int conformerCount() const {
		return E.rows();
	}
	void readFromDistances(const std::string& fileName);
	void setFromEffMatrix(const MatrixXf& m, const std::vector<std::string> &names);
	float rmsdAve(const MatrixXf& rmsds) const
	{
		return rmsdAve(rmsds,_chi2,_nPoints);
	}
	void dumpDistances(const string& fname) const
	{
		std::ofstream outfile;
		outfile.open(fname, std::ifstream::out);
		if(!outfile.is_open())
		{
			return;
		}
		outfile<<"structure";
		for (const auto& name:distNames) {
			outfile<<"\t"<<name;
		}
		outfile<<"\n";
		for (int row=0; row<E.rows(); ++row) {
			outfile<<row<<".pdb";
			for(int col=0; col<E.cols(); ++col) {
				outfile<<"\t";
				if(validity(row,col)) {
					outfile<<EToRda(E(row,col));
				} else {
					outfile<<"nan";
				}
			}
			outfile<<"\n";
		}
	}

	bool dumpChi2(const string& fname) const
	{
		std::ofstream outfile;
		outfile.open(fname, std::ifstream::out);
		if(!outfile.is_open())
		{
			return false;
		}
		dumpChi2(outfile);
		return true;
	}
	std::ostream& dumpChi2(std::ostream &os) const
	{
		os<<std::setprecision(2);
		os<<std::fixed;
		MatrixXf chi2=_chi2;
		chi2.triangularView<Eigen::StrictlyLower>().setZero();
		os<<chi2;
	}
	int bestDistance(const Eigen::MatrixXf& rmsds, const FRETEfficiencies& Eall) const
	{
		//const int numConf=Eall.conformerCount();
		const int numCols=Eall.E.cols();
		std::vector<float> rowAvRmsd(numCols);

		initChi2sfs(distanceCount()+1,std::min(_numFitParams,distanceCount()));

		std::vector<std::thread> threads(std::thread::hardware_concurrency());
		const int grainSize=numCols/threads.size()+1;
		for(int t=0; t<threads.size(); ++t) {
			threads[t]=std::thread([&,t,this] {
				MatrixXf chi2=_chi2, nPoints=_nPoints;
				const int maxD=std::min((t+1)*grainSize,numCols);
				for (int d=t*grainSize; d<maxD; ++d) {
					const auto& dChi2=deltaChi2(Eall.E.col(d),Eall.validity.col(d));
					const auto& dNp=deltaNumPoints(Eall.validity.col(d));
					chi2.noalias()+=dChi2;
					nPoints.noalias()+=dNp;
					rowAvRmsd[d]=rmsdAve(rmsds,chi2,nPoints);
					chi2.noalias()-=dChi2;
					nPoints.noalias()-=dNp;
				}
			});
		}
		for(auto&& t : threads) {
			t.join();
		}
		auto it = std::min_element(rowAvRmsd.begin(), rowAvRmsd.end());
		int best=std::distance(rowAvRmsd.begin(), it);
		return best;
	}
	int worstDistance(const Eigen::MatrixXf& rmsds) const
	{
		const int numCols=E.cols();
		std::vector<float> rowAvRmsd(numCols);
		/*for (int d=0; d<beforeCol; ++d) {
			const auto& dChi2=deltaChi2(E.col(d),validity.col(d));
			const auto& dNp=deltaNumPoints(validity.col(d));
			c2B.noalias()-=dChi2;
			nPointsB.noalias()-=dNp;
			rowAvRmsd[d]=rmsdAve(rmsds,c2B,nPointsB);
			c2B.noalias()+=dChi2;
			nPointsB.noalias()+=dNp;
		}*/
		std::vector<std::thread> threads(std::thread::hardware_concurrency());
		const int grainSize=numCols/threads.size()+1;
		for(int t=0; t<threads.size(); ++t) {
			threads[t]=std::thread([&,this,t] {
				MatrixXf c2=_chi2, nPoints=_nPoints;
				const int maxD=std::min((t+1)*grainSize,numCols);
				for (int d=t*grainSize; d<maxD; ++d) {
					const auto& dChi2=deltaChi2(E.col(d),validity.col(d));
					const auto& dNp=deltaNumPoints(validity.col(d));
					c2.noalias()-=dChi2;
					nPoints.noalias()-=dNp;
					rowAvRmsd[d]=rmsdAve(rmsds,c2,nPoints);
					c2.noalias()+=dChi2;
					nPoints.noalias()+=dNp;
				}
			});
		}
		for(auto&& t : threads) {
			t.join();
		}
		auto it = std::min_element(rowAvRmsd.begin(), rowAvRmsd.end());
		int worst=std::distance(rowAvRmsd.begin(), it);
		return worst;
	}
	void removeDistance(int col)
	{
		const auto& dChi2=deltaChi2(E.col(col),validity.col(col));
		const auto& dNp=deltaNumPoints(validity.col(col));
		_chi2.noalias()-=dChi2;
		_nPoints.noalias()-=dNp;

		const int lastCol=E.cols()-1;
		E.col(col)=E.col(lastCol);
		validity.col(col)=validity.col(lastCol);
		distNames[col]=distNames[lastCol];
		E.conservativeResize(Eigen::NoChange,lastCol);
		validity.conservativeResize(Eigen::NoChange,lastCol);
		distNames.resize(lastCol);
		initChi2sfs(E.cols(),std::min(int(E.cols()-1),_numFitParams));
	}

	/*void initMatrices(const int numConf)
	{
		if(E.rows()==numConf) {
			return;
		}
		E.resize(numConf,0);
		validity.resize(numConf,0);
		_chi2.resize(numConf,numConf);
		_nPoints.resize(numConf,numConf);
		_chi2.setZero();
		_nPoints.setZero();
	}*/

	void addDistance(const FRETEfficiencies& Eall, const int col )
	{
		const auto& dChi2=deltaChi2(Eall.E.col(col),Eall.validity.col(col));
		const auto& dNp=deltaNumPoints(Eall.validity.col(col));
		_chi2.noalias()+=dChi2;
		_nPoints.noalias()+=dNp;

		const int lastCol=E.cols();
		E.conservativeResize(Eigen::NoChange,lastCol+1);
		validity.conservativeResize(Eigen::NoChange,lastCol+1);
		distNames.push_back(Eall.distNames[col]);

		E.col(lastCol)=Eall.E.col(col);
		validity.col(lastCol)=Eall.validity.col(col);
		initChi2sfs(E.cols(),std::min(int(E.cols()-1),_numFitParams));
	}
	int addDistance(const FRETEfficiencies& Eall,const std::string& dist)
	{
		const auto it=std::find(Eall.distNames.begin(), Eall.distNames.end(), dist);
		if(it==Eall.distNames.end()) {
			return -1;
		}
		const int col=std::distance(Eall.distNames.begin(),it);
		addDistance(Eall,col);
		return col;
	}

private:
	void initChi2sfs(const int numDist, const int numFitParams) const
	{
		sfs.clear();
		sfs.reserve(numDist+numFitParams+1);
		for(int i=0;i<numFitParams+1;++i) {
			sfs.push_back(InterpChi2sf());
		}
		for(int i=1; i<=numDist; ++i) {
			try {
				sfs.push_back(InterpChi2sf(i));
			} catch (std::exception &e) {
				std::cerr<<"ERROR! This should never happen! std::exception: " << e.what() << std::endl;
			} catch (boost::exception& e) {
				std::cerr<<"ERROR! This should never happen! boost::exception: " << boost::diagnostic_information(e) << std::endl;
			} catch (...) {
				std::cerr<<"ERROR! This should never happen! exception unknown" << std::endl;
			}
		}
	}

	float rmsdAve(const MatrixXf& rmsds,
		      const MatrixXf& chi2,
		      const MatrixXf& nPoints) const
	{
		const auto& WM=weights(chi2,nPoints);
		VectorXf normVec=WM.rowwise().sum().unaryExpr(
				       [](const float& v) {return v==0.0f?0.0f:1.0f/v;});
		MatrixXf normW(WM.rows(),WM.cols());
		normW.noalias()=normVec.asDiagonal()*WM;
		float nsum=normW.sum();
		if(nsum==0.0f) {
			normW.setOnes();
			nsum=normW.rows()*normW.cols();
		}
		return rmsds.cwiseProduct(normW).sum()/nsum;
	}
	float RdaToE(const float& Rda) const
	{
		return 1.0f/(1.0f+std::pow(Rda/R0,6.0));
	}
	float EToRda(const float& E) const
	{
		return R0*std::pow(1.0/E-1.0,1.0/6.0);
	}
	void chi2()
	{
		const int numECols=E.cols();
		const int numConf=E.rows();
		const auto& Es=E.leftCols(numECols);
		const auto& valSel=validity.leftCols(numECols);

		MatrixXf chi2(numConf,numConf);
		chi2.setZero();
		VectorXf validRow(numECols),contribs(numECols);
		const float invErr=1.0f/err;
		for (int iRef=0; iRef<numConf; ++iRef) {
			for (int j=iRef; j<numConf; ++j) {
				validRow=valSel.row(j).cwiseProduct(valSel.row(iRef));
				contribs=((Es.row(iRef)-Es.row(j))*invErr).cwiseAbs2();
				chi2(iRef,j)=contribs.cwiseProduct(validRow).sum();
			}
		}
		_chi2 = std::move(chi2);
	}
	MatrixXf deltaChi2(const VectorXf& Ecol,const VectorXf& valCol) const
	{
		VectorXf identVec=VectorXf::Constant(Ecol.size(),1.0f);
		MatrixXf M(Ecol.size(),Ecol.size());
		M.noalias()=Ecol*identVec.transpose();

		const float invErr=1.0f/err;
		MatrixXf res(Ecol.size(),Ecol.size());
		res.triangularView<Eigen::Upper>()=
				valCol.asDiagonal()
				* ((M-M.transpose().eval())*invErr).cwiseAbs2()
				* valCol.asDiagonal();
		return std::move(res);
	}
	void numPoints()
	{
		const int numECols=E.cols();
		const MatrixXf& valSel=validity.leftCols(numECols);
		const int numConf=validity.rows();
		MatrixXf numPoints(numConf,numConf);
		numPoints.setZero();
		VectorXf validRow;
		for (int iRef=0; iRef<numConf; ++iRef) {
			for (int j=iRef; j<numConf; ++j) {
				validRow=valSel.row(j).cwiseProduct(valSel.row(iRef));
				numPoints(iRef,j) = std::lround(validRow.sum());
			}
		}
		_nPoints = std::move(numPoints);
	}
	static MatrixXf deltaNumPoints(const VectorXf& valCol)
	{
		return std::move(valCol*valCol.transpose());
	}

	MatrixXf weights(const MatrixXf& chi2,const MatrixXf& numPoints) const
	{
		const int numConf=chi2.rows();
		MatrixXf W(numConf,numConf);
		W.triangularView<Eigen::Upper>()=chi2.binaryExpr(numPoints,
								 [this](const float& c2, const float& np) {
			if(std::lround(np)==0) {
				return 1.0f;
			}
			int ndof=std::lround(np)-_numFitParams;
			ndof=std::max(ndof,1);
			//return 1.0f-float(boost::math::cdf(boost::math::chi_squared(ndof),c2));
			return 1.0f-float(chi2CdfApprox(ndof,c2));

			//TODO: implement fast calculation
			//return sfs[std::lround(np)].value(c2);

		});
		return std::move(W.selfadjointView<Eigen::Upper>());
	}
};

Eigen::MatrixXf readRmsds(const std::string& fileName)
{
	std::ifstream in(fileName);
	if (!in.is_open()) {
		std::cerr<<"Error opening file: "+fileName<<std::endl;
		return Eigen::MatrixXf();
	}

	std::string line;
	std::getline(in,line);
	const int numConf=std::count(line.begin(), line.end()-1, '\t')+1;

	in.seekg(0);
	Eigen::MatrixXf rmsds(numConf,numConf);
	for (int i=0; i<numConf; ++i) {
		for (int j=0; j<numConf; ++j) {
			in>>rmsds(i,j);
		}
	}
	return std::move(rmsds*10.0f);
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

void FRETEfficiencies::readFromDistances(const std::string &fileName/*,const std::unordered_set<std::string> skipDist*/)
{
	std::ifstream in(fileName);
	if (!in.is_open()) {
		std::cerr<<"Error opening file: "+fileName<<std::endl;
		return;
	}
	string line;
	string header;
	std::getline(in,header);
	distNames=split(header,'\t');
	distNames.erase(distNames.begin());
	/*for (int i=0; i<distNames.size(); ++i) {
		if(skipDist.count(distNames[i])>0) {
			distNames.erase(distNames.begin()+i);
			--i;
		}
	}*/
	const int numDist=distNames.size();
	initChi2sfs(numDist,_numFitParams);

	char c;
	std::vector<Eigen::VectorXf> rows;
	std::vector<Eigen::VectorXf> validRows;
	Eigen::VectorXf row(numDist);
	Eigen::VectorXf validRow(numDist);
	while(std::getline(in,line,'\t')) {
		for(int i=0; i<numDist;++i) {
			in>>line;
			if (line=="nan" || line == "-nan") {
				row[i]=0.0f;
				validRow[i]=0.0f;
			}
			else {
				row[i]=std::stof(line);
				validRow[i]=1.0f;
			}
		}
		in>>c;//endline
		rows.push_back(row);
		validRows.push_back(validRow);
	}
	E.resize(rows.size(),numDist);
	validity.resize(validRows.size(),numDist);
	for(int i=0;i<rows.size();i++) {
		E.row(i)=rows[i].unaryExpr(
				 [this](float v) { return RdaToE(v); });
		validity.row(i)=validRows[i];
	}
	chi2();
	numPoints();
}

void FRETEfficiencies::setFromEffMatrix(const FRETEfficiencies::MatrixXf &m, const std::vector<std::string>& names)
{
	if(names.empty()) {
		return;
	}

	distNames=names;
	const int numDist=distNames.size();
	initChi2sfs(numDist,_numFitParams);

	E=m.unaryExpr([](float val) { return std::isnan(val)?0.0f:val;});
	validity=m.unaryExpr([](float val) { return std::isnan(val)?0.0f:1.0f; });

	chi2();
	numPoints();
}

void greedyElimination(const FRETEfficiencies& Eall, const Eigen::MatrixXf& RMSDs)
{
	using std::cout;
	using std::endl;
	using std::flush;
	using std::vector;
	using std::string;
	using Eigen::MatrixXf;
	using Eigen::VectorXf;
	using Eigen::NoChange;

	FRETEfficiencies E=Eall;

	cout<<"#\tRemoved distance\t<<RMSD>>\n";
	cout<<E.distanceCount()<<"\tNONE\t"<<E.rmsdAve(RMSDs)<<endl;
	pushTime("calc <<RMSD>> "+std::to_string(E.distanceCount()));
	for (int i=E.distanceCount()-1; i>0; --i) {
		int w=E.worstDistance(RMSDs);
		pushTime("worstDistance");
		cout<<i<<"\t"<<E.name(w);
		E.removeDistance(w);
		pushTime("removeDistance");
		cout<<"\t"<<E.rmsdAve(RMSDs)<<endl;
		pushTime("calc <row<RMSD>> "+std::to_string(i));
	}
	cout<<"Distances left "<<E.distanceCount()<<": ";
	for(int c=0; c<E.distanceCount(); ++c) {
		cout<<E.name(c)<<" ";
	}
	cout<<endl;
}

void greedySelection(const FRETEfficiencies& Eall, const Eigen::MatrixXf& RMSDs, const float err)
{
	using std::cout;
	using std::endl;
	using std::flush;
	using std::vector;
	using std::string;
	using Eigen::MatrixXf;
	using Eigen::VectorXf;
	using Eigen::NoChange;

	std::stringstream ss;

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
}

void checkSet(const std::vector<std::string>& dists,
	      const FRETEfficiencies& Eall,
	      const Eigen::MatrixXf& RMSDs, const float err,
	      const std::string fname="")
{
	using std::cout;
	using std::endl;
	using std::string;
	using Eigen::MatrixXf;

	FRETEfficiencies E(err,Eall.conformerCount());

	cout<<"Distance added\t<<RMSD>>\n";
	for(const string& name:dists) {
		E.addDistance(Eall,name);
		pushTime("add dist "+name);
		cout<<name<<"\t"<<E.rmsdAve(RMSDs)<<endl;
		pushTime("<<RMSD>>");
	}
	if(!fname.empty()) {
		E.dumpChi2(fname);
	}
}
std::map<std::string,std::string> parameters(const int argc, char *argv[])
{
	std::map<std::string,std::string> map;
	for(int i=1; i<argc-1; i+=2)
	{
		if(argv[i][0]=='-') {
			std::string name,val;
			name=argv[i]+1;
			val=argv[i+1];
			map[name]=val;
		}
	}
	return map;
}
/*
int main(int argc, char *argv[])
{
	using std::vector;
	using std::string;
	using Eigen::MatrixXf;
	using std::cout;
	using std::map;
	using std::endl;

	map<string,string> params=parameters(argc,argv);
	const vector<string> commands={"check","elimination","selection"};
	string usage="-d \"distances.csv\" -rmsds \"rmsds.dat\" -cmd check|elimination|selection [-err 0.058] [-chi2 chi2.dat]";
	if (params.count("d")==0) {
		std::cout<<"distance file is not specified. Usage:\n"<<usage<<endl;
		return 1;
	}
	if (params.count("cmd")==0) {
		std::cout<<"Command is not specified. Usage:\n"<<usage<<endl;
		return 1;
	}
	float err=0.058;
	if (params.count("err")>0) {
		err=std::stof(params["err"]);
	}
	FRETEfficiencies Eall(err);
	pushTime("start");
	Eall.readFromDistances(params["d"]);
	pushTime("readFromDistances");

	if (params.count("rmsds")==0) {
		std::cout<<"RMSD file is not specified. Usage:\n"<<usage<<endl;
		return 1;
	}

	const auto cmdIt = std::find(commands.begin(), commands.end(), params["cmd"]);
	if (cmdIt==commands.end()) {
		std::cout<<"Unknown command. Usage:\n"<<usage<<endl;
		return 1;
	}


	const MatrixXf RMSDs = readRmsds(params["rmsds"]);
	if (RMSDs.cols()!=Eall.conformerCount()) {
		std::cerr<<"RMSDs.cols()!=Eall.conformerCount() "<<RMSDs.cols()<<
			   "!="<<Eall.conformerCount()<<endl;

		return 1;
	}
	pushTime("read RMSDs");

	if (params["cmd"]=="check") {
		vector<string> dists;
		string dist;
		std::cout<<"please enter names of distances to check:"<<endl;
		while(std::cin>>dist) {
			if(dist.empty()) {
				break;
			}
			dists.push_back(dist);
		}
		pushTime("enter distance names");
		if (params.count("chi2")>0) {
			checkSet(dists, Eall, RMSDs,err,params["chi2"]);
		} else {
			checkSet(dists, Eall, RMSDs,err);
		}

		pushTime("checkSet");

	}

	if (params["cmd"]=="elimination") {
		greedyElimination(Eall, RMSDs);
	}

	if (params["cmd"]=="selection") {
		greedySelection(Eall, RMSDs,err);
	}

	return 0;

	printTimes();
}*/
/*const vector<string> forSel0Dists = {"17D_100A", "44D_100A", "1D_100A", "60D_100A", "7D_100A", "41D_100A", "73D_100A", "24D_100A", "56D_100A", "76D_100A"};//forward selection numFitParam=0
cout<<"forSel0Dists:\n";
checkSet(forSel0Dists,Eall,RMSDs);

const vector<string> MIDists = {"28D_100A", "13D_129A", "31D_117A", "56D_126A", "1D_133A", "47D_133A", "44D_110A", "24D_110A", "41D_123A", "64D_133A"};//MI
cout<<"MIDists:\n";
checkSet(MIDists,Eall,RMSDs);

const vector<string> forSel1Dists = {"17D_100A", "44D_100A", "56D_123A", "1D_100A", "67D_100A", "7D_100A", "41D_100A", "76D_100A", "24D_100A", "4D_100A"};//forward selection numFitParam=1
cout<<"forSel1Dists:\n";
checkSet(forSel1Dists,Eall,RMSDs);

const vector<string> elim0Dists = {"17D_133A", "24D_117A", "1D_133A", "34D_117A", "50D_123A", "41D_97A", "56D_93A", "60D_120A", "24D_136A", "50D_133A"};// elimination numFitParam=?
cout<<"elim0Dists:\n";
checkSet(elim0Dists,Eall,RMSDs);*/
#endif // BEST_DIST_H