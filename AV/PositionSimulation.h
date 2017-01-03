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
	PositionSimulation();
	virtual ~PositionSimulation();
	virtual int loadLegacy(std::istream& /*is*/){return -1;}
	virtual PositionSimulationResult
	calculate(const Eigen::Vector3f& attachmentAtomPos,
		  const std::vector<Eigen::Vector4f>& xyzW);
	virtual PositionSimulationResult
	calculate(unsigned atom_i,
		  const std::vector<Eigen::Vector4f>& xyzW) = 0;// v.d.Waals radii
	static PositionSimulation* create(const Position::SimulationType &simulationType);
	virtual PositionSimulation *Clone() = 0;

	using Setting=AbstractEvaluator::Setting;
	virtual Setting setting(int row) const = 0;
	virtual void setSetting(int row, const QVariant& val) = 0;
	virtual int settingsCount() const = 0;

protected:
	const float vdWRMax=6.0f;
};

class PositionSimulationAV3: public PositionSimulation
{
public:
	int loadLegacy(std::istream& is) {
		int pdbId;
		is >> linkerLength >> linkerWidth >> radius[0] >>radius[1]
				>>radius[2]>>pdbId;
		return pdbId;
	}
	Setting setting(int row) const;
	virtual void setSetting(int row, const QVariant& val);
	virtual int settingsCount() const {
		return 12;
	}
	virtual PositionSimulation *Clone(){
		return new PositionSimulationAV3(*this);
	}
	using PositionSimulation::calculate;
	virtual PositionSimulationResult
	calculate(unsigned atom_i,const std::vector<Eigen::Vector4f>& xyzW);

private:
	double gridResolution=0.4;
	double linkerLength=0.0;
	double linkerWidth=0.0;
	float radius[3]={0.0,0.0,0.0};
	double allowedSphereRadius=0.5;
	double allowedSphereRadiusMin=0.5;
	double allowedSphereRadiusMax=2.0;
	double minVolumeSphereFraction=0.0;
	double contactR=0.0;
	double trappedFrac=-1.0;

	const int linknodes=3;
};

class PositionSimulationAV1: public PositionSimulation
{
public:
	PositionSimulationAV1()=default;
	int loadLegacy(std::istream& is){
		int pdbId;
		is >> linkerLength >> linkerWidth >> radius >>pdbId;
		return pdbId;
	}

	Setting setting(int row) const;
	virtual void setSetting(int row, const QVariant& val);
	virtual int settingsCount() const {
		return 10;
	}
	virtual PositionSimulation *Clone(){
		return new PositionSimulationAV1(*this);
	}
	using PositionSimulation::calculate;
	virtual PositionSimulationResult calculate(unsigned atom_i, const std::vector<Eigen::Vector4f>& xyzW);

private:
	double gridResolution=0.4;
	double linkerLength=0.0;
	double linkerWidth=0.0;
	double radius=0.0;
	double allowedSphereRadius=0.5;
	double allowedSphereRadiusMin=0.5;
	double allowedSphereRadiusMax=2.0;
	double minVolumeSphereFraction=0.0;
	double contactR=0.0;
	double trappedFrac=-1.0;

	const int linknodes=3;
};

class PositionSimulationAtom: public PositionSimulation
{
public:

	PositionSimulationAtom()=default;

	Setting setting(int /*row*/) const
	{
		return {"",""};
	}
	virtual void setSetting(int /*row*/, const QVariant& /*val*/) {}

	virtual int settingsCount() const {
		return 0;
	}
	virtual PositionSimulation *Clone(){
		return new PositionSimulationAtom(*this);
	}
	using PositionSimulation::calculate;
	virtual PositionSimulationResult calculate(unsigned atom_i,
						   const std::vector<Eigen::Vector4f>& xyzW)
	{
		std::vector<Eigen::Vector4f> vec(1);
		vec[0]=xyzW[atom_i];
		vec[0][3]=1.0f;
		return PositionSimulationResult(std::move(vec));
	}

private:
};

#endif // POSITIONSIMULATION_H
