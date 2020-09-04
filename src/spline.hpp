#ifndef SPLINE_H
#define SPLINE_H

//#pragma GCC optimize("O3")

#include "polynomial.hpp"
#include <Eigen/Dense>

template <unsigned Deg> class Spline
{
private:
	using MatrixDXf = Eigen::Matrix<float, Deg + 1, Eigen::Dynamic>;
	using MatrixXDf = Eigen::Matrix<float, Eigen::Dynamic, Deg + 1>;
	MatrixDXf c;
	float xmin, xmax;
	float invdx;
	Spline() = default;
	void setFromSorted(const Eigen::Matrix2Xf &sortedXY,
			   const unsigned NPieces);
	inline float value(int i, float x) const;
	static Eigen::VectorXf values(const Eigen::VectorXf &x,
				      const MatrixXDf &cPerm);

public:
	inline float value_unsafe(const float &x) const
	{
		int i = (x - xmin) * invdx;
		assert(i >= 0);
		assert(i < c.cols());
		return value(i, x);
	}
	float value_or(const float &x, const float &def) const
	{
		if (x < xmin || x > xmax) {
			return def;
		}
		int i = (x - xmin) * invdx;
		return value(i, x);
	}
	float value_or(const float &x, const float defLeft,
		       const float defRight) const
	{
		if (x < xmin) {
			return defLeft;
		}
		if (x > xmax) {
			return defRight;
		}
		int i = (x - xmin) * invdx;
		return value(i, x);
	}
	Eigen::VectorXf values_unsafe(const Eigen::VectorXf &x) const;

	unsigned maxAbsDiffArg(const Eigen::Matrix2Xf &xy) const
	{
		float max = 0.0f;
		unsigned arg = 0;
		for (unsigned i = 0; i < xy.cols(); ++i) {
			float diff =
				std::fabs(xy(1, i) - value_unsafe(xy(0, i)));
			if (max < diff) {
				max = diff;
				arg = i;
			}
		}
		return arg;
	}
	static Spline<Deg> fromSorted(const Eigen::Matrix2Xf &sortedXY,
				      const unsigned NPieces);

	template <unsigned Nd>
	friend std::ostream &operator<<(std::ostream &os,
					const Spline<Nd> &spline);
};
template <unsigned Deg>
std::ostream &operator<<(std::ostream &os, const Spline<Deg> &spline)
{
	for (int i = 0; i < spline.polynomials.size(); ++i) {
		float dx = 1.0 / spline.invdx;
		float from = spline.xmin + dx * i;
		float to = from + dx;
		os << '[' << from << ".." << to << "] " << spline.polynomials[i]
		   << '\n';
	}
	return os;
}

template <unsigned int Deg>
Spline<Deg> Spline<Deg>::fromSorted(const Eigen::Matrix2Xf &sortedXY,
				    const unsigned NPieces)
{
	Spline<Deg> sp;
	sp.setFromSorted(sortedXY, NPieces);
	return sp;
}
template <unsigned int Deg>
void Spline<Deg>::setFromSorted(const Eigen::Matrix2Xf &sortedXY,
				const unsigned NPieces)
{
	const int nPoints = sortedXY.cols();
	assert(nPoints > Deg);

	xmin = sortedXY(0, 0);
	xmax = sortedXY(0, nPoints - 1);
	constexpr float inf = std::numeric_limits<float>::infinity();
	invdx = float(NPieces) / (xmax - xmin);
	invdx = nextafterf(invdx, -inf);
	const float dx = 1.0f / invdx;

	assert((xmax - xmin) * invdx < NPieces);

	c.resize(3, NPieces);
	int iStart = 0;
	for (int piece = 0; piece < NPieces - 1; ++piece) {
		float xEnd = dx * (piece + 1) + xmin;
		int iEnd = iStart;
		while (sortedXY(0, iEnd) < xEnd) {
			++iEnd;
		}
		int len = std::max(iEnd - iStart, int(Deg + 1));
		Polynomial<Deg> p(sortedXY.middleCols(iStart, len));
		c.col(piece) = p.c;
		if (iEnd > iStart + Deg) {
			iStart = std::min(int(nPoints - Deg - 1), iEnd);
		}
	}

	Polynomial<Deg> p(sortedXY.middleCols(iStart, nPoints - iStart));
	c.col(NPieces - 1) = p.c;
}

template <> float Spline<0>::value(int i, float) const
{
	return c(0, i);
}

template <> float Spline<1>::value(int i, float x) const
{
	return c(0, i) + c(1, i) * x;
}

template <> float Spline<2>::value(int i, float x) const
{
	return c(0, i) + c(1, i) * x + c(2, i) * x * x;
}

template <unsigned int Deg> float Spline<Deg>::value(int i, float x) const
{
	float val = c(0, i) + c(1, i) * x + c(2, i) * x * x;
	for (int e = 3; e <= Deg; e++) {
		val += c(e, i) * std::pow(x, e);
	}
	return val;
}

template <unsigned int Deg>
Eigen::VectorXf Spline<Deg>::values_unsafe(const Eigen::VectorXf &x) const
{
	// x<xmin is not verified at runtime!
	assert(x.minCoeff() >= xmin);
	const int maxCol = c.cols() - 1;
	const unsigned N = x.size();

	Eigen::VectorXi idxs =
		((x.array() - xmin) * invdx).cast<int>().min(maxCol);
	MatrixXDf slc(N, Deg + 1);
	for (int i = 0; i < N; ++i) {
		slc.row(i) = c.col(idxs(i));
	}
	return values(x, slc);
}

template <unsigned int Deg>
Eigen::VectorXf Spline<Deg>::values(const Eigen::VectorXf &x,
				    const Spline::MatrixXDf &cPerm)
{
	Eigen::VectorXf res = Spline<2>::values(x, cPerm);
	for (int e = 3; e <= Deg; ++e) {
		res += cPerm.col(e).cwiseProduct(x.array().pow(e).matrix());
	}
	return res;
}

template <>
Eigen::VectorXf Spline<2>::values(const Eigen::VectorXf &x,
				  const Spline::MatrixXDf &cPerm)
{
	return cPerm.col(0) + cPerm.col(1).cwiseProduct(x)
	       + cPerm.col(2).cwiseProduct(x.cwiseAbs2());
}

template <>
Eigen::VectorXf Spline<1>::values(const Eigen::VectorXf &x,
				  const Spline::MatrixXDf &cPerm)
{
	return cPerm.col(0) + cPerm.col(1).cwiseProduct(x);
}

template <>
Eigen::VectorXf Spline<0>::values(const Eigen::VectorXf &x,
				  const Spline::MatrixXDf &cPerm)
{
	return cPerm.col(0);
}
#endif // SPLINE_H
