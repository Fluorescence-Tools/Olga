#ifndef POSITIONSIMULATIONRESULT_H
#define POSITIONSIMULATIONRESULT_H
#include <vector>
#include <iostream>
#include <array>
#include <limits>
#include <fstream>

#include <Eigen/Dense>

#include <boost/multi_array.hpp>

class PositionSimulationResult
{
public:

	PositionSimulationResult()
	{
	}
	PositionSimulationResult(std::vector<Eigen::Vector4f>&& points)
	{
		_points=std::forward<std::vector<Eigen::Vector4f>>(points);
	}

	Eigen::Vector3f meanPosition() const;
	double meanFretEfficiency(const PositionSimulationResult& other, const double R0) const;
	double Rda(const PositionSimulationResult& other, unsigned nsamples=200000) const;
	double Rdae(const PositionSimulationResult& other, double R0, unsigned nsamples=200000) const;
	double Rmp(const PositionSimulationResult& other) const;
	double modelDistance(const PositionSimulationResult& other, const std::string& type, double R0=0) const;
	double minDistance(const PositionSimulationResult& other) const
	{
		float minDistSq=std::numeric_limits<double>::infinity();
		for(const Eigen::Vector4f& p1:_points) {
			for (const Eigen::Vector4f& p2:other._points) {
				minDistSq=std::min(minDistSq,(p2-p1).head<3>().squaredNorm());
			}
		}
		return std::sqrt(minDistSq);
	}
	std::ostream& dump_xyz(std::ostream& os) const;
	std::ostream& dumpShellXyz(std::ostream& os) const;
	bool dumpXyz(const std::string& fileName) const;
	bool dumpShellXyz(const std::string& fileName) const;
	bool empty() const
	{
		return !_points.size();
	}
	size_t size() const
	{
		return _points.size();
	}
	size_t freeSize() const
	{
		size_t size=0;
		for(const auto& point:_points) {
			if(point[3]==1.0f) {
				++size;
			}
		}
		return size;
	}
	void translate(const Eigen::Vector3f r) {
		for(auto& point:_points) {
			point.head(3)+=r;
		}
		_meanPosition+=r;
	}
	float overlap(const std::vector<Eigen::Vector3f>& refs, float maxR) const
	{
		if (_points.size()==0) {
			return std::numeric_limits<double>::quiet_NaN();
		}
		float totalVol=0.0f;
		float overlapVol=0.0f;
		const float maxRSq=maxR*maxR;
		//Eigen::Vector3f mp=meanPosition();
		for (const Eigen::Vector4f& point: _points) {
			/*float rMpSq=(mp-point.head<3>()).squaredNorm();
			float w=(std::exp(-rMpSq/(2.0f*10.0f*10.0f))+1.0f)*0.5f;*/
			totalVol+=point[3];
			for (const Eigen::Vector3f& ref: refs ) {
				float rSq=(ref-point.head<3>()).squaredNorm();
				if(rSq<maxRSq) {
					overlapVol+=point[3];
					break;
				}
			}
		}
		return overlapVol/totalVol;
	}

protected:
	typedef boost::multi_array<bool, 3> densityArray_t;
	typedef densityArray_t::extent_range densityArrayRange_t;
	//Maps a vector of points to 3-dimensional density grid with
	//A layer of empty cells around the grid, which is added to avoid boundary-check
	//so in the end size of the grid is a bit bigger, then needed to accomodate all points
	densityArray_t pointsToDensity(double res=0.5) const;
	static bool allNeighboursFilled(const densityArray_t& arr, int i, int j, int k);
	std::vector<Eigen::Vector3f> shell(double res=0.5) const;
	std::vector<Eigen::Vector4f> _points;
	static constexpr double nan=std::numeric_limits<double>::quiet_NaN();
	mutable Eigen::Vector3f _meanPosition={nan,nan,nan};
};
namespace std {
inline std::string to_string(const PositionSimulationResult& res)
{
	Eigen::Vector3f mp=res.meanPosition();
	std::stringstream ss;
	ss<<mp.transpose();
	return ss.str();
}

}

#endif // POSITIONSIMULATIONRESULT_H
