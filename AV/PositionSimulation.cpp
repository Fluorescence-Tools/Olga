#include "PositionSimulation.h"
#include "fretAV.h"
#include "Position.h"

PositionSimulation::PositionSimulation()
{
}

PositionSimulation::~PositionSimulation()
{
}

PositionSimulation* PositionSimulation::create(const Position::SimulationType &simulationType)
{
	switch(simulationType)
	{
	case Position::SimulationType::AV1:
		return new PositionSimulationAV1;
	case Position::SimulationType::AV3:
		return new PositionSimulationAV3;
	case Position::SimulationType::ATOM:
		return new PositionSimulationAtom;
	}
	return nullptr;
}

PositionSimulation::Setting PositionSimulationAV3::setting(int row) const
{
	switch(row)
	{
	case 0:
		return Setting{"simulation_grid_resolution",gridResolution};
	case 1:
		return Setting{"linker_length",linkerLength};
	case 2:
		return Setting{"linker_width",linkerWidth};
	case 3:
		return Setting{"radius1",radius[0]};
	case 4:
		return Setting{"radius2",radius[1]};
	case 5:
		return Setting{"radius3",radius[2]};
	case 6:
		return Setting{"allowed_sphere_radius",allowedSphereRadius};
	case 7:
		return Setting{"allowed_sphere_radius_min",allowedSphereRadiusMin};
	case 8:
		return Setting{"allowed_sphere_radius_max",allowedSphereRadiusMax};
	case 9:
		return Setting{"min_sphere_volume_fraction",minVolumeSphereFraction};
	case 10:
		return Setting{"contact_volume_thickness",contactR};
	case 11:
		return Setting{"contact_volume_trapped_fraction",trappedFrac};
	}
	return Setting();
}

void PositionSimulationAV3::setSetting(int row, const QVariant &val)
{
	switch(row)
	{
	case 0:
		gridResolution=val.toDouble();
		return;
	case 1:
		linkerLength=val.toDouble();
		return;
	case 2:
		linkerWidth=val.toDouble();
		return;
	case 3:
		radius[0]=val.toDouble();
		return;
	case 4:
		radius[1]=val.toDouble();
		return;
	case 5:
		radius[2]=val.toDouble();
		return;
	case 6:
		allowedSphereRadius=val.toDouble();
		return;
	case 7:
		allowedSphereRadiusMin=val.toDouble();
		return;
	case 8:
		allowedSphereRadiusMax=val.toDouble();
		return;
	case 9:
		minVolumeSphereFraction=val.toDouble();
		return;
	case 10:
		contactR=val.toDouble();
		return;
	case 11:
		trappedFrac=val.toDouble();
		return;
	}
}

PositionSimulationResult
PositionSimulation::calculate(const Eigen::Vector3f &attachmentAtomPos,
			      const std::vector<Eigen::Vector4f> &xyzW)
{
	Eigen::Vector4f v(attachmentAtomPos(0),attachmentAtomPos(1),attachmentAtomPos(2),0.0f);
	int atom_i=-1;
	for(unsigned i=0; i<xyzW.size(); i++)
	{
		v[3]=xyzW[i][3];
		if( (v-xyzW.at(i)).squaredNorm()<0.01f ) {
			atom_i=i;
			break;
		}
	}
	if(atom_i<0) {
		std::cerr<<"error, attachment atom is not found in the stripped molecule\n"<<std::flush;
		return PositionSimulationResult();
	}
	//TODO: this is a horrible hack
	std::vector<Eigen::Vector4f> xyzW2=xyzW;
	xyzW2[atom_i][3]=0.0f;
	return calculate(atom_i,xyzW2);
}

PositionSimulationResult
PositionSimulationAV3::calculate(unsigned atom_i,
				 const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector4f> res=
			calculateAV3(xyzW,xyzW[atom_i],linkerLength,
				     linkerWidth,{radius[0],radius[1],radius[2]},
		     gridResolution,contactR,trappedFrac);
	double volfrac=res.size()/(4.0/3.0*3.14159*std::pow(linkerLength/gridResolution,3.0));
	if (minVolumeSphereFraction>volfrac) {
		res.clear();
	}
	return PositionSimulationResult(std::move(res));
}

PositionSimulation::Setting PositionSimulationAV1::setting(int row) const
{
	switch(row)
	{
	case 0:
		return Setting{"simulation_grid_resolution",gridResolution};
	case 1:
		return Setting{"linker_length",linkerLength};
	case 2:
		return Setting{"linker_width",linkerWidth};
	case 3:
		return Setting{"radius1",radius};
	case 4:
		return Setting{"allowed_sphere_radius",allowedSphereRadius};
	case 5:
		return Setting{"allowed_sphere_radius_min",allowedSphereRadiusMin};
	case 6:
		return Setting{"allowed_sphere_radius_max",allowedSphereRadiusMax};
	case 7:
		return Setting{"min_sphere_volume_fraction",minVolumeSphereFraction};
	case 8:
		return Setting{"contact_volume_thickness",contactR};
	case 9:
		return Setting{"contact_volume_trapped_fraction",trappedFrac};
	}
	return Setting();
}

void PositionSimulationAV1::setSetting(int row, const QVariant &val)
{
	switch(row)
	{
	case 0:
		gridResolution=val.toDouble();
		return;
	case 1:
		linkerLength=val.toDouble();
		return;
	case 2:
		linkerWidth=val.toDouble();
		return;
	case 3:
		radius=val.toDouble();
		return;
	case 4:
		allowedSphereRadius=val.toDouble();
		return;
	case 5:
		allowedSphereRadiusMin=val.toDouble();
		return;
	case 6:
		allowedSphereRadiusMax=val.toDouble();
		return;
	case 7:
		minVolumeSphereFraction=val.toDouble();
		return;
	case 8:
		contactR=val.toDouble();
		return;
	case 9:
		trappedFrac=val.toDouble();
		return;
	}
}

PositionSimulationResult PositionSimulationAV1::calculate(unsigned atom_i, const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector4f> res=calculateAV(xyzW,xyzW[atom_i],linkerLength,linkerWidth,
		     radius,gridResolution,contactR,trappedFrac);
	double volfrac=res.size()/(4.0/3.0*3.14159*std::pow(linkerLength/gridResolution,3.0));
	if (minVolumeSphereFraction>volfrac) {
		res.clear();
	}
	return PositionSimulationResult(std::move(res));
}
