#ifndef ABSTRACTCALCRESULT_H
#define ABSTRACTCALCRESULT_H
#include <string>
#include <memory>
#include <QMetaType>
class AbstractCalcResult
{
public:
	virtual std::string toString() const = 0;
	virtual unsigned int columnsCount() const = 0;
};
Q_DECLARE_METATYPE(std::shared_ptr<AbstractCalcResult>);

#endif // ABSTRACTCALCRESULT_H
