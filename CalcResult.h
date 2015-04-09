#ifndef CALCRESULT_H
#define CALCRESULT_H

#include <Eigen/Core>
#include <Eigen/Dense>

#include "AbstractCalcResult.h"

namespace std {
inline std::string to_string(const Eigen::Matrix4d& matrix)
{
	std::stringstream ss;
	ss<<matrix;
	return ss.str();
}

}

template <class T>
class CalcResult: public AbstractCalcResult
{
private:
	T _value;
public:
	CalcResult(T val)
	{
		_value=val;
	}
	std::string toString(int /*i=0*/) const
	{
		return std::to_string(_value);
	}
	unsigned int columnsCount() const
	{
		return 1;
	}
	const T& get() const
	{
		return _value;
	}
};
template <>
inline std::string CalcResult<Eigen::Vector3d>::toString(int i)const
{
	return std::to_string(_value[i]);
}

/*
template <>
class CalcResult<int>:public AbstractCalcResult
{
private:
	int value;
public:
	CalcResult(int _val)
	{
		value=_val;
	}
	std::string toString(int column=0) const
	{
		return std::to_string(value)+" (int)";
	}
	unsigned int columnsCount() const
	{
		return 1;
	}
};*/
#endif // CALCRESULT_H
