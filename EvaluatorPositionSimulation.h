#ifndef EVALUATORPOSITIONSIMULATION_H
#define EVALUATORPOSITIONSIMULATION_H

#include "AbstractEvaluator.h"
#include "AV/Position.h"
#include <QVariantMap>

class EvaluatorPositionSimulation : public AbstractEvaluator
{
private:
	Position _position;
	std::shared_ptr<AbstractCalcResult> calculate(const pteros::System &system,const FrameDescriptor &frame) const;
public:
	EvaluatorPositionSimulation(const TaskStorage& storage,
				    const std::string& name):
		AbstractEvaluator(storage),_position(name) {}
	EvaluatorPositionSimulation(const TaskStorage& storage,
				    const QString &legacy,
				    const QString &pdbFileName):
		EvaluatorPositionSimulation(storage,"") {
		_position.setFromLegacy(legacy.toStdString(),pdbFileName.toStdString());
	}

	virtual Task
	makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _position.name();
	}
	virtual std::string columnName(int) const
	{
		return std::string("AV ")+_position.name()+"";
	}
	virtual int columnCount() const
	{
		return 0;
	}
	virtual std::string className() const {
		return "Positions";
	}
	virtual std::pair<QString,QVariant> setting(int row) const
	{
		return _position.setting(row);
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		_position.setSetting(row,val);
	}
	virtual void setName(const std::string& name)
	{
		_position.setName(name);
	}

	virtual int settingsCount() const
	{
		return _position.settingsCount();
	}
	std::string anchorAtoms() const {
		return _position.anchorAtoms();
	}
	//~EvaluatorPositionSimulation();
};

#endif // EVALUATORPOSITIONSIMULATION_H
