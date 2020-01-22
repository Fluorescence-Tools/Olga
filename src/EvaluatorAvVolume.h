#ifndef EVALUATORAVVOLUME_H
#define EVALUATORAVVOLUME_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"

class EvaluatorAvVolume : public AbstractEvaluator
{
private:
	EvalId _av;
	std::string _name;
	bool _onlyContact = false;

public:
	EvaluatorAvVolume(const TaskStorage &storage, const std::string &name)
	    : AbstractEvaluator(storage), _name(name)
	{
	}
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _name;
	}
	virtual std::string className() const
	{
		return "AV Size";
	}
	virtual std::string columnName(int) const
	{
		return name();
	}
	virtual int columnCount() const
	{
		return 1;
	}
	virtual int settingsCount() const
	{
		return 2;
	}
	virtual Setting setting(int row) const override
	{
		switch (row) {
		case 0: {
			EvalId id =
				_storage.isValid(_av)
					? _av
					: _storage.evaluatorPositionSimulation;
			return {"position_name", QVariant::fromValue(id)};
		}
		case 1:
			return {"contact_only_volume", _onlyContact};
		}
		return {"", ""};
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		using std::cerr;
		switch (row) {
		case 0: {
			auto tmpId = val.value<EvalId>();
			if (tmpId == EvalId(-1)) {
				cerr << "Can not set position option <"
				                + val.toString().toStdString()
				                + "> for evaluator " + _name
				                + "\n"
				     << std::flush;
			} else {
				_av = tmpId;
			}
			return;
		}
		case 1:
			_onlyContact = val.toBool();
			return;
		}
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
	calculate(const PositionSimulationResult &av) const;
};

#endif // EVALUATORAVVOLUME_H
