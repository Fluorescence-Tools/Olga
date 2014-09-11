#ifndef POSITIONSIMULATIONRESULT_H
#define POSITIONSIMULATIONRESULT_H
#include <vector>
#include <iostream>
#include <array>
#include <limits>

#include <Eigen/Dense>


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
    /*bool dump_xyz(const std::string& fileName) {
        std::ofstream outfile;
        outfile.open(fileName.toLocal8Bit().data(), std::ifstream::out);
        if(!outfile.is_open())
        {
            return false;
        }
        dump_xyz(outfile);
        outfile.close();
        return true;
    }*/
    bool empty() const
    {
        return !_points.size();
    }

protected:
    std::vector<Eigen::Vector3f> _points;
    mutable Eigen::Vector3f _meanPosition;
};

#endif // POSITIONSIMULATIONRESULT_H
