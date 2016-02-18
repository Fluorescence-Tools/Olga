#ifndef POSITION_H
#define POSITION_H
#include <QVariantMap>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
//#include "PositionSimulation.h"
#include "PositionSimulationResult.h"
#include "AbstractEvaluator.h"
#include "av_routines.h"
#include <iostream>
#include <vector>

#include "pteros/pteros.h"

class Distance;
class PositionSimulation;

class Position
{
public:
	enum class SimulationType {av1, av3, atom};
	static SimulationType simulationType(const std::string& str)
	{
		if(str=="AV1") {
			return SimulationType::av1;
		} else if (str=="AV3") {
			return SimulationType::av3;
		} else if (str=="ATOM") {
			return SimulationType::atom;
		}
		return SimulationType::av1;
	}
	static std::string simulationTypeName(const SimulationType& type) {
		switch (type) {
		case SimulationType::av1:
			return "AV1";
		case SimulationType::av3:
			return "AV3";
		case SimulationType::atom:
			return "ATOM";
		default:
			return "AV1";
		}
	}

	explicit Position();
	virtual ~Position();
	Position(const Position& other);
	Position& operator=(const Position& other);
	Position(Position &&o);
	Position& operator=(Position&& o);

	//Position(const QJsonObject& positionJson, const std::string& name);
	Position(const QVariantMap& positionJson, const std::string& name);

	PositionSimulationResult calculate(const pteros::System &system) const;

	//virtual QJsonObject jsonObject() const;
	//virtual QString settingName(int row) const;
	//virtual QVariant settingValue(int row) const;
	std::pair<QString,QVariant> setting(int row) const;
	void setSetting(int row,const QVariant& val);
	virtual int settingsCount() const;
	//bool load(const QJsonObject& positionJson, const std::string& name);
	bool load(const QVariantMap& positionJson, const std::string& name);
	const std::string& name() const;
	void setName(const std::string& name);

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
	void setStripMask(const std::string& stripMask) {
		_stripMask=stripMask;
	}
	std::string anchorAtoms() const {
		return _anchorAtoms;
	}

	SimulationType simulationType() const;
	void setSimulationType(const SimulationType &simulationType);

	static double obsolete_chi2(pteros::System &system,
				   const std::vector<Position> &positions,
				   const std::vector<Distance> &distances);
	static std::vector<Position> fromLegacy(const std::string& labelingFileName,
						const std::string& pdbFileName);
	static QJsonObject jsonObjects(const std::vector<std::shared_ptr<Position> > &arr);
	Eigen::Vector3f atomXYZ(const pteros::System &system) const;

	void setFromLegacy(const std::string& entry,const std::string &pdbFileName);
private:
	//Eigen::Vector3f atomXYZ(BALL::System &system) const;

	PositionSimulationResult calculate(const Eigen::Vector3f& attachmentAtomPos,
					   const std::vector<Eigen::Vector4f>& store) const;

private:
	std::string _name;
	std::string _chainIdentifier;
	unsigned _residueSeqNumber;
	std::string _residueName;
	std::string _atomName;
	std::string _stripMask;
	std::string _anchorAtoms;
	SimulationType _simulationType;
	const int _localSettingCount=7;
	PositionSimulation* _simulation;
};
Q_DECLARE_METATYPE(Position::SimulationType)
#endif // POSITION_H
