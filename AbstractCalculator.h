#ifndef ABSTRACTCALCULATOR_H
#define ABSTRACTCALCULATOR_H
#include <list>
#include <memory>
#include <unordered_map>

#include <pteros/pteros.h>

#include "AbstractCalcResult.h"
#include "FrameDescriptor.h"

class AbstractCalculator
{
protected:
	using ResultCacheCol=std::unordered_map<std::shared_ptr<AbstractCalculator>,
	std::shared_ptr<AbstractCalcResult>>;
	using ResultCache=std::unordered_map<FrameDescriptor,ResultCacheCol>;
private:
	static unsigned calcCounter;
	const unsigned _id=calcCounter++;
	const ResultCache& _results;
protected:
	std::shared_ptr<AbstractCalcResult> result(const FrameDescriptor &desc, const std::weak_ptr<AbstractCalculator> calculator) const
	{
		if(auto calc=calculator.lock()) {
			auto cacherow=_results.find(desc);
			if(cacherow==_results.end()) {
				return std::shared_ptr<AbstractCalcResult>();
			}
			auto iresult=cacherow->second.find(calc);
			if(iresult==cacherow->second.end()){
				return std::shared_ptr<AbstractCalcResult>();
			}
			return iresult->second;
		}
		return std::shared_ptr<AbstractCalcResult>();
	}

	static pteros::System getSystem(const FrameDescriptor& desc)
	{
		pteros::System sys;
		sys.load(desc.topologyFileName());
		return sys;
	}
public:
	virtual ~AbstractCalculator()
	{
	}
	AbstractCalculator(const ResultCache& results):_results(results)
	{}
	/*virtual std::shared_ptr<AbstractCalcResult>
		calculate(const pteros::System& system) const=0;*/
	virtual std::shared_ptr<AbstractCalcResult>
			calculate(const FrameDescriptor& desc) const=0;
	virtual unsigned id() const final
	{
		return _id;
	}
	virtual std::string name(int i=0) const=0;
};
Q_DECLARE_METATYPE(std::shared_ptr<AbstractCalculator>)
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
