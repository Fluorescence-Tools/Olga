#ifndef ABSTRACTCALCULATOR_H
#define ABSTRACTCALCULATOR_H
#include <list>
#include <memory>

#include <pteros/pteros.h>

#include "AbstractCalcResult.h"


class AbstractCalculator
{
private:
	static unsigned calcCounter;
	const unsigned _id=calcCounter++;
public:
	virtual ~AbstractCalculator()
	{
	}
	virtual std::shared_ptr<AbstractCalcResult>
		calculate(const pteros::System& system) const=0;
	virtual unsigned id() const final
	{
		return _id;
	}
	virtual std::string name() const=0;
};
bool inline operator==(const AbstractCalculator& lhs, const AbstractCalculator& rhs) {
    return lhs.id() == rhs.id();
}
/*namespace std {
template <>
struct hash<AbstractCalculator*> {
	size_t operator()(const AbstractCalculator* calc) const {
		return std::hash<unsigned>()(calc->id());
	}
};
}

namespace std {
template <>
struct hash<std::reference_wrapper<AbstractCalculator>> {
  size_t operator()(const AbstractCalculator& x) const {
    return std::hash<unsigned>()(x.id());
  }
};
}
bool inline operator==(const AbstractCalculator& lhs, const AbstractCalculator& rhs) {
    return lhs.id() == rhs.id();
}

	std::vector<std::unique_ptr<AbstractCalculator>> calculators;
	CalcCache cache;

	calculators.push_back(std::make_unique<CalculatorPi>());
	calculators.push_back(std::make_unique<CalculatorAnswer>());

	for(const auto& clc:calculators)//pipeline imitation
	{
		cache[*clc]=clc->calculate("test");
	}

	for(const auto& itm:cache)//view imitation
	{
		std::cout<<itm.first.get().id()<<" "<<itm.second->toString()<<std::endl;
	}
*/
#endif // ABSTRACTCALCULATOR_H
