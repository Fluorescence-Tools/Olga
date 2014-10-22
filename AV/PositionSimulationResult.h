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
		_meanPosition.fill(std::numeric_limits<double>::quiet_NaN());
	}
	PositionSimulationResult(std::vector<Eigen::Vector3f>&& points)
	{
		_points=std::forward<std::vector<Eigen::Vector3f>>(points);
	}

	Eigen::Vector3f meanPosition() const;
	double Rda(const PositionSimulationResult& other, unsigned nsamples=100000) const;
	double Rdae(const PositionSimulationResult& other, double R0, unsigned nsamples=100000) const;
	double Rmp(const PositionSimulationResult& other) const;
	double modelDistance(const PositionSimulationResult& other, const std::string& type, double R0=0) const;
	std::ostream& dump_xyz(std::ostream& os);
	std::ostream& dumpShellXyz(std::ostream& os);
	bool dumpXyz(const std::string& fileName);
	bool dumpShellXyz(const std::string& fileName);
	bool empty() const
	{
		return !_points.size();
	}
	size_t size() const
	{
		return _points.size();
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
	std::vector<Eigen::Vector3f> _points;
	mutable Eigen::Vector3f _meanPosition;
};

#endif // POSITIONSIMULATIONRESULT_H
