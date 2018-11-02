#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <Eigen/Dense>
#include <iosfwd>

template <unsigned D> class Polynomial
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
public:
	Eigen::Matrix<float, D + 1, 1> c;

	static Eigen::Matrix<float, D + 1, 1> powers(const float &x);
	inline float value(const float &x) const
	{
		return c.cwiseProduct(powers(x)).sum();
	}
	Polynomial(const Eigen::Matrix2Xf &xy);
	Polynomial() = default;
	template <unsigned Deg>
	friend std::ostream &operator<<(std::ostream &os,
					const Polynomial<Deg> &p);
};

template <unsigned D> Polynomial<D>::Polynomial(const Eigen::Matrix2Xf &xy)
{
	const int N = xy.cols();
	assert(N > 0);
	Eigen::Matrix<float, Eigen::Dynamic, D + 1> X(N, D + 1);
	for (int i = 0; i < N; ++i) {
		X.row(i) = powers(xy(0, i));
	}
	c = X.householderQr().solve(xy.row(1).transpose());
	// c = X.fullPivHouseholderQr().solve(xy.row(1).transpose());
}


template <unsigned D>
std::ostream &operator<<(std::ostream &os, const Polynomial<D> &p)
{
	os << p.c[0];
	if (D > 0) {
		os << " + " << p.c[1] << " * x";
	}
	for (int i = 2; i < D + 1; ++i) {
		os << " + " << p.c[i] << " * x^" << i;
	}
	return os;
}

template <> Eigen::Matrix<float, 2, 1> Polynomial<1>::powers(const float &x)
{
	return Eigen::Matrix<float, 2, 1>(1.0f, x);
}

template <> Eigen::Matrix<float, 3, 1> Polynomial<2>::powers(const float &x)
{
	return Eigen::Matrix<float, 3, 1>(1.0f, x, x * x);
}

template <> Eigen::Matrix<float, 4, 1> Polynomial<3>::powers(const float &x)
{
	return Eigen::Matrix<float, 4, 1>(1.0f, x, x * x, x * x * x);
}

template <unsigned int D>
Eigen::Matrix<float, D + 1, 1> Polynomial<D>::powers(const float &x)
{
	Eigen::Matrix<float, D + 1, 1> vec;
	vec[0] = 1.0f;
	for (auto i = 1; i < D + 1; ++i) {
		vec[i] = vec[i - 1] * x;
	}
	return vec;
}
#endif // POLYNOMIAL_H
