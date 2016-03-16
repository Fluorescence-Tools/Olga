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
		_meanPosition.fill(0.0);
		for(const Eigen::Vector3f& point:_points)
		{
			_meanPosition+=point;
		}
		_meanPosition/=static_cast<double>(_points.size());
	}
	return _meanPosition;
}

double PositionSimulationResult::Rda(const PositionSimulationResult &other, unsigned nsamples) const
{
	using std::min;
	unsigned long av1length=_points.size();
	unsigned long av2length=other._points.size();
	const unsigned long rndLim=(av1length-1)*(av2length-1);
	double mean=0.0;
	if(nsamples<(av1length*av2length) && nsamples!=0)//MC sampling
	{
		std::random_device rd;
		std::mt19937 engine(rd());
		double r = 0.;
		unsigned long i1, i2;
		std::uniform_int_distribution<unsigned long> dist(0,rndLim);
		nsamples/=2;
		for(unsigned i=0; i<nsamples; i++)
		{
			i1 = dist(engine);
			i2 = dist(engine);
			r += (_points.at(i1%av1length)-other._points.at(i2%av2length)).norm();
			r += (_points.at(i2%av1length)-other._points.at(i1%av2length)).norm();
		}
		nsamples*=2;
		mean = r/static_cast<double>(nsamples);
		return mean;
	}
	else//explicit sampling
	{
		double r=0.0;
		for(unsigned i=0; i<av1length; i++)
		{
			for(unsigned j=0; j<av2length; j++)
			{
				r += (_points.at(i)-other._points.at(j)).norm();
			}

		}
		mean = r/static_cast<double>(av1length*av2length);
		return mean;
	}
	return mean;
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

		nsamples/=2;
		for(unsigned i=0; i<nsamples; i++)
		{
			i1 = distr(engine);
			i2 = distr(engine);
			r2 = (_points.at(i1%av1length)-other._points.at(i2%av2length)).squaredNorm();
			e += 1. / (1. + r2 * r2 * r2 * R0r6);
			r2 = (_points.at(i2%av1length)-other._points.at(i1%av2length)).squaredNorm();
			e += 1. / (1. + r2 * r2 * r2 * R0r6);
		}
		e /= static_cast<double>(nsamples*2);
	}
	else//explicit sampling
	{
		for(unsigned i=0; i<av1length; i++)
		{
			for(unsigned j=0; j<av2length; j++)
			{
				r2 = (_points.at(i)-other._points.at(j)).squaredNorm();
				e += 1. / (1. + r2 * r2 * r2 * R0r6);
			}

		}
		e /= static_cast<double>(av1length*av2length);
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
		os<<"AV ";
		os<<_points.at(i)[0]<<"\t";
		os<<_points.at(i)[1]<<"\t";
		os<<_points.at(i)[2]<<"\n";
	}
	meanPosition();
	os<<"AVmp "<<_meanPosition[0]<<"\t"<<_meanPosition[1]<<"\t"<<_meanPosition[2]<<"\n";
	os.flags(osflags);
	return os;
}

std::ostream &PositionSimulationResult::dumpShellXyz(std::ostream &os)
{
	std::vector<Eigen::Vector3f> _shell=shell();
	int n=_shell.size();
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

bool PositionSimulationResult::dumpShellXyz(const std::string &fileName) {
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
	Eigen::Vector3f minLWD(_points.at(0));
	Eigen::Vector3f maxLWD(_points.at(0));
	for(const auto& point: _points)
	{
		minLWD=minLWD.array().min(point.array());
		maxLWD=maxLWD.array().max(point.array());
	}

	densityArrayRange_t rangeI(minLWD(0)/res-1,maxLWD(0)/res+1);
	densityArrayRange_t rangeJ(minLWD(1)/res-1,maxLWD(1)/res+1);
	densityArrayRange_t rangeK(minLWD(2)/res-1,maxLWD(2)/res+1);
	auto extents=boost::extents[rangeI][rangeJ][rangeK];
	densityArray_t density(extents);
	std::fill(density.data(), density.data() + density.num_elements(), false);
	for(const auto& point: _points)
	{
		Eigen::Vector3i xyz=(point/res).cast<int>();
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
