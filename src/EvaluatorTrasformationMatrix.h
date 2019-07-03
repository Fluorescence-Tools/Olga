#ifndef EVALUATORTRANSORMATIONMATRIX_H
#define EVALUATORTRANSORMATIONMATRIX_H
#include <QMetaType>
#include "AbstractEvaluator.h"
#include "TaskStorage.h"
#include <Eigen/Dense>

class EvaluatorTrasformationMatrix : public AbstractEvaluator
{
private:
	// MolecularSystemDomain _domain;
	std::vector<std::pair<std::string, Eigen::Vector3d>> comSellPos;
	std::string _name;
	int numPoints() const
	{
		return comSellPos.size();
	}

public:
	EvaluatorTrasformationMatrix(const TaskStorage &storage,
				     const std::string &name);
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;

	virtual std::string name() const
	{
		return _name;
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}
	virtual std::string className() const
	{
		return "Coordinate systems";
	}
	virtual int columnCount() const
	{
		return 0;
	}
	virtual std::string columnName(int i) const
	{
		(void)i;
		return "none";
	}
	virtual int settingsCount() const
	{
		return 2 * numPoints() + 1;
	}
	virtual Setting setting(int row) const;
	virtual void setSetting(int row, const QVariant &val);


private:
	std::shared_ptr<AbstractCalcResult>
	calculate(const pteros::System &system) const;
};
// Q_DECLARE_METATYPE( Eigen::Vector3d )
#endif // EVALUATORTRANSORMATIONMATRIX_H
