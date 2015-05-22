#ifndef ABSTRACTCALCRESULT_H
#define ABSTRACTCALCRESULT_H
#include <string>
#include <memory>
#include <QMetaType>
class AbstractCalcResult
{
public:
	virtual std::string toString(int i=0) const = 0;
	virtual unsigned int columnsCount() const = 0;
	virtual ~AbstractCalcResult(){}
};
Q_DECLARE_METATYPE(std::shared_ptr<AbstractCalcResult>)

#endif // ABSTRACTCALCRESULT_H
