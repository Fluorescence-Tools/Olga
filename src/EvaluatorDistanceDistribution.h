#ifndef DISTANCEDISTRIBUTION_H
#define DISTANCEDISTRIBUTION_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"
#include <mutex>

class EvaluatorDistanceDistribution : public AbstractEvaluator
{
private:
	EvalId _av1 = _storage.evaluatorPositionSimulation, _av2 = _av1;
	std::string _name, _histPath;
	double _distMin = 0.0, _distMax = 100.0, _binSize = 1.0;
	mutable std::ofstream outfile;
	mutable std::mutex writeMtx;
	std::string fullPath(const std::string &path) const;
	void writeString(const std::string &str) const;
	bool initOut() const;

public:
	EvaluatorDistanceDistribution(const TaskStorage &storage,
				      const std::string &name);
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
	virtual std::string name() const
	{
		return _name;
	}
	virtual std::string className() const
	{
		return "Distance distribution";
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
		return 6;
	}
	virtual Setting setting(int row) const override
	{
		switch (row) {
		case 0: {
			EvalId id =
				_storage.isValid(_av1)
					? _av1
					: _storage.evaluatorPositionSimulation;
			return {"position1_name", QVariant::fromValue(id)};
		}
		case 1: {
			EvalId id =
				_storage.isValid(_av2)
					? _av2
					: _storage.evaluatorPositionSimulation;
			return {"position2_name", QVariant::fromValue(id)};
		}
		case 2:
			return {"histograms_path",
				QString::fromStdString(fullPath(_histPath))};
		case 3:
			return {"min_distance", _distMin};
		case 4:
			return {"max_distance", _distMax};
		case 5:
			return {"bin_size", _binSize};
		}
		return {"", ""};
	}
	virtual void setSetting(int row, const QVariant &val)
	{
		switch (row) {
		case 0: {
			auto tmpId = val.value<EvalId>();
			if (tmpId == EvalId(-1)) {
				std::cerr << "Can not set position 1\n"
					  << std::flush;
				return;
			}
			_av1 = tmpId;
			return;
		}
		case 1: {
			auto tmpId = val.value<EvalId>();
			if (tmpId == EvalId(-1)) {
				std::cerr << "Can not set position 2\n"
					  << std::flush;
				return;
			}
			_av2 = tmpId;
			return;
		}
		case 2:
			_histPath = fullPath(val.toString().toStdString());
			return;
		case 3:
			_distMin = val.toDouble();
			return;
		case 4:
			_distMax = val.toDouble();
			return;
		case 5:
			_binSize = val.toDouble();
			return;
		}
	}
	virtual void setName(const std::string &name)
	{
		_name = name;
	}

private:
	virtual std::shared_ptr<AbstractCalcResult>
	calculate(const PositionSimulationResult &av1,
		  const PositionSimulationResult &av2,
		  const std::string traj) const;
};

#endif // DISTANCEDISTRIBUTION_H
