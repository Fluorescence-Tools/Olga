#include "PositionSimulationResult.h"
#include <random>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <cmath>

Eigen::Vector3f PositionSimulationResult::meanPosition() const
{

	if(std::isnan(_meanPosition(0)))
	{
		Eigen::Vector3f tmp;
		tmp.fill(0.0);
		double totalW=0.0;
		for(const Eigen::Vector4f& point:_points)
		{
			tmp+=point.head<3>()*point[3];
			totalW+=point[3];
		}
		tmp/=totalW;
		_meanPosition=tmp;
	}
	return _meanPosition;
}

double PositionSimulationResult::meanFretEfficiency(const PositionSimulationResult &other, const double R0) const
{
	const unsigned long nsamples=40000;

	unsigned long av1length=_points.size();
	unsigned long av2length=other._points.size();
	const unsigned long rndLim=(av1length-1)*(av2length-1);
	std::random_device rd;
	std::mt19937 engine(rd());
	double eff = 0.0;
	unsigned long i1, i2;
	std::uniform_int_distribution<unsigned long> dist(0,rndLim);
	double totalW=0.0;
	for(unsigned i=0; i<nsamples/2; i++)
	{
		i1 = dist(engine);
		i2 = dist(engine);

		double w=_points.at(i1%av1length)[3]*other._points.at(i2%av2length)[3];
		totalW+=w;
		double r = (_points.at(i1%av1length)-other._points.at(i2%av2length)).head<3>().norm();
		eff += 1.0/(1.0+std::pow(r/R0,6.0))*w;

		w=_points.at(i2%av1length)[3]*other._points.at(i1%av2length)[3];
		totalW+=w;
		r = (_points.at(i2%av1length)-other._points.at(i1%av2length)).head<3>().norm();
		eff += 1.0/(1.0+std::pow(r/R0,6.0))*w;
	}
	return eff/totalW;
}

double PositionSimulationResult::Rda(const PositionSimulationResult &other, unsigned nsamples) const
{
	using std::min;
	unsigned long av1length=_points.size();
	unsigned long av2length=other._points.size();
	const unsigned long rndLim=(av1length-1)*(av2length-1);

	if(nsamples<(av1length*av2length) && nsamples!=0)//MC sampling
	{
		std::random_device rd;
		std::mt19937 engine(rd());
		double r = 0.;
		unsigned long i1, i2;
		std::uniform_int_distribution<unsigned long> dist(0,rndLim);
		double totalW=0.0;
		for(unsigned i=0; i<nsamples/2; i++)
		{
			i1 = dist(engine);
			i2 = dist(engine);
			float w=_points.at(i1%av1length)[3]*other._points.at(i2%av2length)[3];
			totalW+=w;
			r += (_points.at(i1%av1length)-other._points.at(i2%av2length)).head<3>().norm()*w;
			w=_points.at(i2%av1length)[3]*other._points.at(i1%av2length)[3];
			totalW+=w;
			r += (_points.at(i2%av1length)-other._points.at(i1%av2length)).head<3>().norm()*w;
		}
		return r/totalW;
	}
	else//explicit sampling
	{
		double r=0.0;
		double totalW=0.0;
		for(unsigned i=0; i<av1length; i++)
		{
			for(unsigned j=0; j<av2length; j++)
			{
				float w=_points.at(i)[3]*other._points.at(j)[3];
				totalW+=w;
				r += (_points.at(i)-other._points.at(j)).head<3>().norm()*w;
			}
		}
		return r/totalW;
	}
}

double PositionSimulationResult::Rdae(const PositionSimulationResult &other, double R0, unsigned nsamples) const
{
	using std::min;
	unsigned long av1length=_points.size();
	unsigned long av2length=other._points.size();
	const unsigned long rndLim=(av1length-1)*(av2length-1);
	double r2, e = 0., R0r6 = 1./(R0*R0*R0*R0*R0*R0);
	if(nsamples<av1length*av2length && nsamples!=0)//MC sampling
	{
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_int_distribution<unsigned long> distr(0,rndLim);
		int i1, i2;
		double totalW = 0.0;
		for(unsigned i=0; i<nsamples/2; i++)
		{
			i1 = distr(engine);
			i2 = distr(engine);
			float w = _points.at(i1%av1length)[3]*other._points.at(i2%av2length)[3];
			totalW+=w;
			r2 = (_points.at(i1%av1length)-other._points.at(i2%av2length)).head<3>().squaredNorm();
			e += w / (1. + r2 * r2 * r2 * R0r6);

			w = _points.at(i2%av1length)[3]*other._points.at(i1%av2length)[3];
			totalW+=w;
			r2 = (_points.at(i2%av1length)-other._points.at(i1%av2length)).head<3>().squaredNorm();
			e += w / (1. + r2 * r2 * r2 * R0r6);
		}
		e /= totalW;
	}
	else//explicit sampling
	{
		double totalW = 0.0;
		for(unsigned i=0; i<av1length; i++)
		{
			for(unsigned j=0; j<av2length; j++)
			{
				float w =_points.at(i)[3]*other._points.at(j)[3];
				totalW+=w;
				r2 = (_points.at(i)-other._points.at(j)).head<3>().squaredNorm();
				e += w / (1. + r2 * r2 * r2 * R0r6);
			}

		}
		e /= totalW;
	}
	return R0 * pow((1./e - 1.), 1./ 6.);
}

double PositionSimulationResult::Rmp(const PositionSimulationResult &other) const
{
	Eigen::Vector3f mp1=meanPosition();
	Eigen::Vector3f mp2=other.meanPosition();
	double distMp=0.0;
	for (int i=0;i<3;i++)
	{
		double dq=mp1[i]-mp2[i];
		distMp+=dq*dq;
	}
	return sqrt(distMp);
}

double PositionSimulationResult::modelDistance(const PositionSimulationResult &other, const std::string &type, double R0) const
{
	if(type=="RDAMean")
	{
		return Rda(other);
	}
	else if (type=="Rmp")
	{
		return Rmp(other);
	}
	else if(type=="RDAMeanE")
	{
		return Rdae(other,R0);
	}
	//should never reach here;
	std::cerr<<"Distance type is unknown: "<<type<<std::endl;
	return -100.0;
}



std::ostream &PositionSimulationResult::dump_xyz(std::ostream &os) const
{
	int n=_points.size();
	std::ios::fmtflags osflags  = os.flags();
	os<<n+1<<"\n";
	if(n==0)
	{
		os<<"AV cloud is empty, probably the simulation has not been performed\n";
	}
	else
	{
		os<<"comment\n";
	}
	os.unsetf ( std::ios::fixed );
	os<<std::setprecision(5);

	for(int i=0; i < n; i++)
	{
		if (_points.at(i)[3]>1.0f) {
			os<<"AVc ";
		} else {
			os<<"AV ";
		}
		os<<_points.at(i)[0]<<"\t";
		os<<_points.at(i)[1]<<"\t";
		os<<_points.at(i)[2]<<"\n";
	}
	meanPosition();
	os<<"AVmp "<<_meanPosition[0]<<"\t"<<_meanPosition[1]<<"\t"<<_meanPosition[2]<<"\n";
	os.flags(osflags);
	return os;
}
std::ostream &PositionSimulationResult::dump_dxmap(std::ostream &os) const
{
	const float res=2.0f;
	densityArray_t density=pointsToDensity(res);
	int iMin = density.index_bases()[0];
	int jMin = density.index_bases()[1];
	int kMin = density.index_bases()[2];
	int iMax = iMin + density.shape()[0];
	int jMax = jMin + density.shape()[1];
	int kMax = kMin + density.shape()[2];

	os<<"# TEST\nobject 1 class gridpositions counts "
	 <<iMax-iMin<<' '<<jMax-jMin<<' '<<kMax-kMin<<'\n'
	<<"origin "<<res*iMin<<' '<<res*jMin<<' '<<res*kMin<<'\n'
	<<"delta "<<res<<" 0 0\n"
	<<"delta 0 "<<res<<" 0\n"
	<<"delta 0 0 "<<res<<"\n"
	<<"object 2 class gridconnections counts "
	<<iMax-iMin<<' '<<jMax-jMin<<' '<<kMax-kMin<<'\n'
	<<"object 3 class array type double rank 0 items "
	<<(kMax-kMin)*(jMax-jMin)*(iMax-iMin)<<" data follows\n";

	int n=0;
	for(int i=iMin; i<iMax; i++)
		for(int j=jMin; j<jMax; j++)
			for(int k=kMin; k<kMax; k++)
			{
				os<<density[i][j][k];
				++n;
				if(n%3==0) {
					os<<'\n';
				} else {
					os<<' ';
				}
			}
	os<<"\nobject \"density (all) [A^-3]\" class field\n";
	return os;
}

bool PositionSimulationResult::dump_dxmap(const std::string &fileName) const
{
	std::ofstream outfile;
	outfile.open(fileName, std::ifstream::out);
	if(!outfile.is_open())
	{
		return false;
	}
	dump_dxmap(outfile);
	outfile.close();
	return true;
}
std::ostream &PositionSimulationResult::dumpShellXyz(std::ostream &os) const
{
	if(_points.size()==0)
	{
		os<<"AV cloud is empty\n";
		return os;
	}
	std::vector<Eigen::Vector3f> _shell=shell();
	int n=_shell.size();
	std::ios::fmtflags osflags  = os.flags();
	os<<n+1<<"\n";
	os<<"comment\n";
	os.unsetf ( std::ios::fixed );
	os<<std::setprecision(5);

	for(int i=0; i < n; i++)
	{
		os<<"AV ";
		os<<_shell.at(i)[0]<<"\t";
		os<<_shell.at(i)[1]<<"\t";
		os<<_shell.at(i)[2]<<"\n";
	}
	meanPosition();
	os<<"AVmp "<<_meanPosition[0]<<"\t"<<_meanPosition[1]<<"\t"<<_meanPosition[2]<<"\n";
	os.flags(osflags);
	return os;
}

bool PositionSimulationResult::dumpXyz(const std::string &fileName) const {
	std::ofstream outfile;
	outfile.open(fileName, std::ifstream::out);
	if(!outfile.is_open())
	{
		return false;
	}
	dump_xyz(outfile);
	outfile.close();
	return true;
}

bool PositionSimulationResult::dumpShellXyz(const std::string &fileName) const {
	std::ofstream outfile;
	outfile.open(fileName, std::ifstream::out);
	if(!outfile.is_open())
	{
		return false;
	}
	dumpShellXyz(outfile);
	outfile.close();
	return true;
}

PositionSimulationResult::densityArray_t PositionSimulationResult::pointsToDensity(double res) const
{
	//l,w,d
	Eigen::Vector3f minLWD(_points.at(0).head<3>());
	Eigen::Vector3f maxLWD(_points.at(0).head<3>());
	for(const auto& point: _points)
	{
		minLWD=minLWD.array().min(point.head<3>().array());
		maxLWD=maxLWD.array().max(point.head<3>().array());
	}

	densityArrayRange_t rangeI(minLWD(0)/res-1,maxLWD(0)/res+1);
	densityArrayRange_t rangeJ(minLWD(1)/res-1,maxLWD(1)/res+1);
	densityArrayRange_t rangeK(minLWD(2)/res-1,maxLWD(2)/res+1);
	auto extents=boost::extents[rangeI][rangeJ][rangeK];
	densityArray_t density(extents);
	std::fill(density.data(), density.data() + density.num_elements(), false);
	for(const auto& point: _points)
	{
		Eigen::Vector3i xyz=(point/res).head(3).cast<int>();
		density[xyz(0)][xyz(1)][xyz(2)]=true;
	}
	return density;
}

bool PositionSimulationResult::allNeighboursFilled(const PositionSimulationResult::densityArray_t &arr, int i, int j, int k)
{
	bool allNeighboursFilled=1;
	for(int di=-1; di<2; di++)
		for(int dj=-1; dj<2; dj++)
			for(int dk=-1; dk<2; dk++)
			{
				allNeighboursFilled*=arr[i+di][j+dj][k+dk];
			}
	return allNeighboursFilled;
}

std::vector<Eigen::Vector3f> PositionSimulationResult::shell(double res) const
{
	std::vector<Eigen::Vector3f> _shellPoints;
	densityArray_t density=pointsToDensity(res);

	//outermost layers are always filled with 0, so we don't need to check them
	int iMin = density.index_bases()[0] + 1;
	int jMin = density.index_bases()[1] + 1;
	int kMin = density.index_bases()[2] + 1;
	int iMax = iMin + density.shape()[0] - 2;
	int jMax = jMin + density.shape()[1] - 2;
	int kMax = kMin + density.shape()[2] - 2;
	for(int i=iMin; i<iMax; i++)
		for(int j=jMin; j<jMax; j++)
			for(int k=kMin; k<kMax; k++)
			{
				if(density[i][j][k] && !allNeighboursFilled(density,i,j,k))
				{
					Eigen::Vector3f point(i,j,k);
					point*=res;
					_shellPoints.push_back(point);
				}
			}
	return _shellPoints;
}
