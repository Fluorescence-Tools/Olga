#ifndef POSITION_H
#define POSITION_H
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "PositionSimulation.h"
#include "av_routines.h"
#include <iostream>
#include <vector>

#include "pteros/pteros.h"

class Distance;


class Position
{
public:
	explicit Position();
	virtual ~Position();

	Position(const Position& other);
	Position& operator=(const Position& other);
	Position(Position &&o);
	Position& operator=(Position&& o);

	Position(const QJsonObject& positionJson, const std::string& name);

	PositionSimulationResult calculate(const pteros::System &system) const;

	virtual QJsonObject jsonObject() const;
	bool load(const QJsonObject& positionJson, const std::string& name);
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

	std::string simulationType() const;
	void setSimulationType(const std::string &simulationType);
//	static double chi2(BALL::System &system,
//			   const std::vector<Position> &positions,
//			   const std::vector<Distance> &distances);
	static double obsolete_chi2(pteros::System &system,
				   const std::vector<Position> &positions,
				   const std::vector<Distance> &distances);
	static std::vector<Position> fromLegacy(const std::string& labelingFileName,
						const std::string& pdbFileName);
	static QJsonObject jsonObjects(const std::vector<std::shared_ptr<Position> > &arr);
private:
	//Eigen::Vector3f atomXYZ(BALL::System &system) const;
	Eigen::Vector3f atomXYZ(const pteros::System &system) const;
	PositionSimulationResult calculate(const Eigen::Vector3f& attachmentAtomPos,
					   const std::vector<Eigen::Vector4f>& store) const;
	void setFromLegacy(const std::string& entry,const std::string &pdbFileName);

private:
	std::string _name;
	std::string _chainIdentifier;
	unsigned _residueSeqNumber;
	std::string _residueName;
	std::string _atomName;
	std::string _simulationType;
	PositionSimulation* _simulation;
};

#endif // POSITION_H
