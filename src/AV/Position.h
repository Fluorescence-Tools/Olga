#ifndef POSITION_H
#define POSITION_H

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include "PositionSimulationResult.h"
#include "AbstractEvaluator.h"
#include "fretAV.h"

#include <iostream>
#include <vector>

#include "pteros/pteros.h"

class Distance;
class PositionSimulation;

class Position
{
public:
	enum class SimulationType { AV1, AV3, ATOM, NONE };
	static SimulationType simulationType(const std::string &str)
	{
		if (str == "AV1") {
			return SimulationType::AV1;
		} else if (str == "AV3") {
			return SimulationType::AV3;
		} else if (str == "ATOM") {
			return SimulationType::ATOM;
		}
		std::cerr << "simulation type is not supported: " + str + "\n"
			  << std::flush;
		return SimulationType::NONE;
	}
	static std::string simulationTypeName(const SimulationType &type)
	{
		switch (type) {
		case SimulationType::AV1:
			return "AV1";
		case SimulationType::AV3:
			return "AV3";
		case SimulationType::ATOM:
			return "ATOM";
		default:
			return "AV1";
		}
	}

	explicit Position();
	virtual ~Position();
	Position(const Position &other);
	Position &operator=(const Position &other);
	Position(Position &&o);
	Position &operator=(Position &&o);
	Position(const std::string &name);

	PositionSimulationResult calculate(const pteros::System &system) const;

	std::pair<QString, QVariant> setting(int row) const;
	void setSetting(int row, const QVariant &val);
	virtual int settingsCount() const;

	const std::string &name() const;
	void setName(const std::string &name);

	std::string chainIdentifier() const;
	void setChainIdentifier(const std::string &chainIdentifier);

	unsigned residueSeqNumber() const;
	void setResidueSeqNumber(const unsigned &residueSeqNumber);

	std::string residueName() const;
	void setResidueName(const std::string &residueName);

	std::string atomName() const;
	void setAtomName(const std::string &atomName);

	std::string stripMask() const
	{
		return _stripMask;
	}
	void setStripMask(const std::string &stripMask)
	{
		_stripMask = stripMask;
	}
	std::string anchorAtoms() const
	{
		return _anchorAtoms;
	}

	SimulationType simulationType() const;
	void setSimulationType(const SimulationType &simulationType);

	static double obsolete_chi2(pteros::System &system,
				    const std::vector<Position> &positions,
				    const std::vector<Distance> &distances);
	static std::vector<Position>
	fromLegacy(const std::string &labelingFileName,
		   const std::string &pdbFileName);
	static QJsonObject
	jsonObjects(const std::vector<std::shared_ptr<Position>> &arr);
	Eigen::Vector3f atomXYZ(const pteros::System &system) const;

	void setFromLegacy(const std::string &entry,
			   const std::string &pdbFileName);

private:
	// Eigen::Vector3f atomXYZ(BALL::System &system) const;
	std::string selectionExpression() const;
	std::string stripExpression() const;
	PositionSimulationResult
	calculate(const Eigen::Vector3f &attachmentAtomPos,
		  const std::vector<Eigen::Vector4f> &store) const;

private:
	std::string _name;
	std::string _chainIdentifier;
	unsigned _residueSeqNumber = 0;
	std::string _residueName;
	std::string _atomName;
	std::string _stripMask;
	std::string _anchorAtoms;
	double _allowedSphereRadius = 0.0;
	SimulationType _simulationType = SimulationType::AV1;
	const int _localSettingCount = 8;
	PositionSimulation *_simulation = nullptr;
};
Q_DECLARE_METATYPE(Position::SimulationType)
#endif // POSITION_H
