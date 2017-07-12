#ifndef FRETAV_H
#define FRETAV_H
#include <limits>
#include <vector>
#include <Eigen/Dense>

class TabulatedFunction {
	double xMin=0.0, xMax=0.0;
	Eigen::VectorXd y;
public:
	TabulatedFunction(double xMin, double xMax, const Eigen::VectorXd& y):
		xMin(xMin), xMax(xMax), y(y)
	{
	}
	TabulatedFunction() = default;
	double value(const double& x) const
	{
		if (x<xMin || x>=xMax) {
			return std::numeric_limits<double>::quiet_NaN();
		}
		const int i=(x-xMin)*y.size()/(xMax-xMin);
		const double dx=(xMax-xMin)/(y.size()-1.0);
		const double k=(y[i+1]-y[i])/dx;
		const double b=y[i]-k*(xMin+dx*i);
		return k*x+b;
	}
	bool isValid() const
	{
		return y.size();
	}
};

std::vector<Eigen::Vector4f> calculateAV(const std::vector<Eigen::Vector4f> &xyzR,
		     Eigen::Vector4f rSource, float linkerLength,
		     float linkerWidth, float dyeRadius,
		     float discretizationStep, float contactR, float trappedFrac, const TabulatedFunction& weighting);

std::vector<Eigen::Vector4f> calculateAV(const std::vector<Eigen::Vector4f> &xyzR,
		     Eigen::Vector4f rSource, float linkerLength,
		     float linkerWidth, float dyeRadius,
		     float discretizationStep, float contactR, float trappedFrac);

std::vector<Eigen::Vector4f> calculateAV3(const std::vector<Eigen::Vector4f> &xyzR,
					 Eigen::Vector4f rSource, float linkerLength,
					 float linkerWidth, Eigen::Vector3f dyeRadii,
		     float discretizationStep, float contactR, float trappedFrac);
#endif // FRETAV_H
