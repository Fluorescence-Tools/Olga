#ifndef POSITIONSIMULATION_H
#define POSITIONSIMULATION_H
#include <QJsonObject>
#include <Eigen/Dense>

#include <array>
#include <vector>
#include "PositionSimulationResult.h"




class XYZVstore;

class PositionSimulation
{
public:
    PositionSimulation();
    virtual ~PositionSimulation();
    virtual bool load(const QJsonObject& positionJson)=0;
    virtual int loadLegacy(std::istream& /*is*/){return -1;}
    virtual PositionSimulationResult calculate(const Eigen::Vector3f& attachmentAtomPos, const std::vector<Eigen::Vector4f>& xyzW);
    virtual PositionSimulationResult calculate(unsigned atom_i,
					       const std::vector<Eigen::Vector4f>& xyzW)=0;// v.d.Waals radii
    static PositionSimulation* create(const std::string& simulationType);
    static PositionSimulation* create(const QJsonObject& positionJson);
    virtual PositionSimulation *Clone()=0;
    virtual QJsonObject jsonObject()=0;
};

class PositionSimulationAV3: public PositionSimulation
{
public:
    PositionSimulationAV3();
    PositionSimulationAV3(const QJsonObject& positionJson);
    bool load(const QJsonObject& positionJson);
    int loadLegacy(std::istream& is){
	int pdbId;
	    is >> linkerLength >> linkerWidth >> radius[0] >>radius[1]>>radius[2]>>pdbId;
	    return pdbId;
    }
    QJsonObject jsonObject();
    virtual PositionSimulation *Clone(){
	return new PositionSimulationAV3(*this);
    }
    using PositionSimulation::calculate;
    virtual PositionSimulationResult calculate(unsigned atom_i, const std::vector<Eigen::Vector4f>& xyzW);


private:
    double getAllowedSphereRadius(int atom_i,
				   const double *XLocal, const double *YLocal, const double *ZLocal, 		// atom coordinates
				   const double *vdWR, int NAtoms,				// v.d.Waals radii
				   double linkersphere, int linknodes, 					// linker routing parameters
				   unsigned char* density) const;
private:
    double gridResolution;
    double linkerLength;
    double linkerWidth;
    double radius[3];
    double allowedSphereRadius;
    double allowedSphereRadiusMin;
    double allowedSphereRadiusMax;
};

class PositionSimulationAV1: public PositionSimulation
{
public:
    PositionSimulationAV1()=default;
    PositionSimulationAV1(const QJsonObject& positionJson);
    bool load(const QJsonObject& positionJson);
    int loadLegacy(std::istream& is){
	int pdbId;
	    is >> linkerLength >> linkerWidth >> radius >>pdbId;
	    return pdbId;
    }
    QJsonObject jsonObject();
    virtual PositionSimulation *Clone(){
	return new PositionSimulationAV1(*this);
    }
    using PositionSimulation::calculate;
    virtual PositionSimulationResult calculate(unsigned atom_i, const std::vector<Eigen::Vector4f>& xyzW);


private:
    double getAllowedSphereRadius(int atom_i,
				   const double *XLocal, const double *YLocal, const double *ZLocal, 		// atom coordinates
				   const double *vdWR, int NAtoms,				// v.d.Waals radii
				   double linkersphere, int linknodes, 					// linker routing parameters
				   unsigned char* density) const;
private:
    double gridResolution=0.4;
    double linkerLength=0.0;
    double linkerWidth=0.0;
    double radius=0.0;
    double allowedSphereRadius=-1.0;
    double allowedSphereRadiusMin=0.5;
    double allowedSphereRadiusMax=2.0;
};

#endif // POSITIONSIMULATION_H
