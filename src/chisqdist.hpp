//  Copyright John Maddock 2006-7.
//  Copyright Paul A. Bristow 2007.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef CHISQDIST_HPP
#define CHISQDIST_HPP

#include <cmath>
//
// Upper gamma fraction for integer a:
//
inline double finite_gamma_q(unsigned a, double x)
{
	//
	// Calculates normalised Q when a is an integer:
	//
	double e = std::exp(-x);
	double sum = e;
	if (sum != 0.0) {
		double term = sum;
		for (unsigned n = 1; n < a; ++n) {
			term /= n;
			term *= x;
			sum += term;
		}
	}
	return sum;
}
//
// Upper gamma fraction for half integer a:
//
double finite_half_gamma_q(double a, double x)
{
	//
	// Calculates normalised Q when a is a half-integer:
	//
	using std::sqrt;
	double e = std::erfc(sqrt(x));
	constexpr double pi = 3.141592653589793238462643383279502884;
	if ((e != 0.0) && (a > 1.0) && x != 0.0) {
		double term = std::exp(-x) / sqrt(pi * x);
		term *= x;
		term /= 0.5;
		double sum = term;
		for (unsigned n = 2; n < a; ++n) {
			term /= n - 0.5;
			term *= x;
			sum += term;
		}
		e += sum;
	}
	return e;
}
double gamma_q(double a, double x)
{
	if (a > 100.0) {
		return 0.5 * std::erfc((x - a) / std::sqrt(2.0 * a));
	}
	if (int(2 * a) % 2 > 0) {
		return finite_half_gamma_q(a, x);
	} else {
		return finite_gamma_q(a, x);
	}
}
double chisqRTcdf(double chisq, unsigned Ndof)
{
	// return boost::math::gamma_q(Ndof * 0.5, chisq * 0.5f);
	return gamma_q(0.5 * Ndof, 0.5 * chisq);
}

#endif // CHISQDIST_HPP
