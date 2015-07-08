#ifndef POSITIONSIMULATION_H
#define POSITIONSIMULATION_H
#include <Eigen/Dense>

#include <array>
#include <vector>

#include "PositionSimulationResult.h"
#include "Position.h"
#include "AbstractEvaluator.h"



class XYZVstore;
class PositionSimulation
{
public:
	//virtual bool load(const QJsonObject& positionJson)=0;
	//static PositionSimulation* create(const QJsonObject& positionJson);
	//virtual QJsonObject jsonObject() const=0;
	PositionSimulation();
	virtual ~PositionSimulation();
	virtual bool load(const QVariantMap& settings)=0;
	virtual int loadLegacy(std::istream& /*is*/){return -1;}
	virtual PositionSimulationResult
	calculate(const Eigen::Vector3f& attachmentAtomPos,
		 const std::vector<Eigen::Vector4f>& xyzW);
	virtual PositionSimulationResult
	calculate(unsigned atom_i,
		  const std::vector<Eigen::Vector4f>& xyzW)=0;// v.d.Waals radii
	static PositionSimulation* create(const Position::SimulationType &simulationType);
	static PositionSimulation* create(const QVariantMap& settings);
	virtual PositionSimulation *Clone()=0;

	using Setting=AbstractEvaluator::Setting;
	virtual Setting setting(int row) const = 0;
	virtual void setSetting(int row, const QVariant& val) = 0;
	virtual int settingsCount() const = 0;
	virtual bool inline operator==(const PositionSimulation& o) const =0;
protected:
	const float vdWRMax=6.0f;
};

class PositionSimulationAV3: public PositionSimulation
{
public:
	//PositionSimulationAV3(const QJsonObject& positionJson);
	//bool load(const QJsonObject& positionJson);
	//QJsonObject jsonObject() const;

	bool load(const QVariantMap& settings);
	int loadLegacy(std::istream& is) {
		int pdbId;
		is >> linkerLength >> linkerWidth >> radius[0] >>radius[1]
				>>radius[2]>>pdbId;
		return pdbId;
	}
	Setting setting(int row) const;
	virtual void setSetting(int row, const QVariant& val);
	virtual int settingsCount() const {
		return 9;
	}
	virtual PositionSimulation *Clone(){
		return new PositionSimulationAV3(*this);
	}
	using PositionSimulation::calculate;
	virtual PositionSimulationResult
	calculate(unsigned atom_i,const std::vector<Eigen::Vector4f>& xyzW);
	virtual bool inline operator==(const PositionSimulation& o) const
	{
		if(this==&o) {
			return true;
		}
		auto* new_o = dynamic_cast<const PositionSimulationAV3*>(&o);
		if(new_o) {
			if(gridResolution!=new_o->gridResolution
			   || linkerLength!=new_o->linkerLength
			   || linkerWidth!=new_o->linkerWidth
			   || allowedSphereRadius!=new_o->allowedSphereRadius
			   || allowedSphereRadiusMin!=new_o->allowedSphereRadiusMin
			   || allowedSphereRadiusMax!=new_o->allowedSphereRadiusMax
			   || radius[0]!=new_o->radius[0] || radius[1]!=new_o->radius[1]
			   || radius[2]!=new_o->radius[2]) {
				return false;
			}
			return true;
		}
		return false;
	}

private:
	double getAllowedSphereRadius(unsigned atom_i,
				      const std::vector<Eigen::Vector4f> &xyzW,
				      std::vector<Eigen::Vector3f> &res) const;
private:
	double gridResolution=0.4;
	double linkerLength=0.0;
	double linkerWidth=0.0;
	double radius[3]={0.0,0.0,0.0};
	double allowedSphereRadius=-1.0;
	double allowedSphereRadiusMin=0.5;
	double allowedSphereRadiusMax=2.0;

	const int linknodes=3;
};

class PositionSimulationAV1: public PositionSimulation
{
public:
	//PositionSimulationAV1(const QJsonObject& positionJson);
	//bool load(const QJsonObject& positionJson);
	//QJsonObject jsonObject() const;
	PositionSimulationAV1()=default;
	bool load(const QVariantMap& settings);
	int loadLegacy(std::istream& is){
		int pdbId;
		is >> linkerLength >> linkerWidth >> radius >>pdbId;
		return pdbId;
	}

	Setting setting(int row) const;
	virtual void setSetting(int row, const QVariant& val);
	virtual int settingsCount() const {
		return 7;
	}
	virtual PositionSimulation *Clone(){
		return new PositionSimulationAV1(*this);
	}
	using PositionSimulation::calculate;
	virtual PositionSimulationResult calculate(unsigned atom_i, const std::vector<Eigen::Vector4f>& xyzW);
	virtual bool inline operator==(const PositionSimulation& o) const
	{
		if(this==&o) {
			return true;
		}
		auto* new_o = dynamic_cast<const PositionSimulationAV1*>(&o);
		if(new_o) {
			if(gridResolution!=new_o->gridResolution
			   || linkerLength!=new_o->linkerLength
			   || linkerWidth!=new_o->linkerWidth
			   || allowedSphereRadius!=new_o->allowedSphereRadius
			   || allowedSphereRadiusMin!=new_o->allowedSphereRadiusMin
			   || allowedSphereRadiusMax!=new_o->allowedSphereRadiusMax
			   || radius!=new_o->radius) {
				return false;
			}
			return true;
		}
		return false;
	}

private:
	double getAllowedSphereRadius(unsigned atom_i,
				      const std::vector<Eigen::Vector4f> &xyzW,
				      std::vector<Eigen::Vector3f> &res) const;
private:
	double gridResolution=0.4;
	double linkerLength=0.0;
	double linkerWidth=0.0;
	double radius=0.0;
	double allowedSphereRadius=0.5;
	double allowedSphereRadiusMin=0.5;
	double allowedSphereRadiusMax=2.0;

	const int linknodes=3;
};

#endif // POSITIONSIMULATION_H
