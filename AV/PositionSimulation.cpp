#include "PositionSimulation.h"
#include "av_routines.h"

PositionSimulation::PositionSimulation()
{
}

PositionSimulation::~PositionSimulation()
{
}

PositionSimulation* PositionSimulation::create(const std::string &simulationType)
{
	if(simulationType=="AV3")
	{
		return new PositionSimulationAV3;
	}
	std::cerr<<"PositionSimulation creation error!\n"<<std::endl;
	return 0;
}

PositionSimulation* PositionSimulation::create(const QJsonObject &positionJson)
{
	std::string type=positionJson.value("simulation_type").toString().toStdString();
	PositionSimulation* pos=create(type);
	pos->load(positionJson);
	return pos;
}

PositionSimulationAV3::PositionSimulationAV3()
{
	gridResolution=0.4;
	linkerLength=0.0;
	linkerWidth=0.0;
	radius[0]=radius[1]=radius[2]=0.0;
	allowedSphereRadius=-1.0;
	allowedSphereRadiusMin=0.5;
	allowedSphereRadiusMax=2.0;
}

PositionSimulationAV3::PositionSimulationAV3(const QJsonObject &positionJson)
{
	load(positionJson);
}

bool PositionSimulationAV3::load(const QJsonObject &positionJson)
{
	gridResolution=positionJson.value("simulation_grid_resolution").toDouble(0.4);
	linkerLength=positionJson.value("linker_length").toDouble(0.0);
	linkerWidth=positionJson.value("linker_width").toDouble(0.0);
	radius[0]=positionJson.value("radius1").toDouble(0.0);
	radius[1]=positionJson.value("radius2").toDouble(0.0);
	radius[2]=positionJson.value("radius3").toDouble(0.0);
	allowedSphereRadius=positionJson.value("allowed_sphere_radius").toDouble(-1.0);
	allowedSphereRadiusMin=positionJson.value("allowed_sphere_radius_min").toDouble(0.5);
	allowedSphereRadiusMax=positionJson.value("allowed_sphere_radius_max").toDouble(2.0);
	return true;
}

QJsonObject PositionSimulationAV3::jsonObject()
{
	QJsonObject position;
	position.insert("simulation_grid_resolution",gridResolution);
	position.insert("linker_length",linkerLength);
	position.insert("linker_width",linkerWidth);
	position.insert("radius1",radius[0]);
	position.insert("radius2",radius[1]);
	position.insert("radius3",radius[2]);
	position.insert("allowed_sphere_radius",allowedSphereRadius);
	position.insert("allowed_sphere_radius_min",allowedSphereRadiusMin);
	position.insert("allowed_sphere_radius_max",allowedSphereRadiusMax);
	return position;
}

double PositionSimulationAV3::
getAllowedSphereRadius(int atom_i, const double *XLocal, const double *YLocal,
		       const double *ZLocal, const double *vdWR, int NAtoms,
		       double linkersphere, int linknodes,
		       unsigned char *density) const
{
	if(allowedSphereRadius>=0.0)
	{
		return allowedSphereRadius;
	}
	(void)atom_i; (void)XLocal; (void)YLocal; (void)ZLocal; (void)vdWR;
	(void)NAtoms; (void)linkersphere; (void)linknodes; (void)density;
	//TODO: add automatic sphere radius determination
	return allowedSphereRadiusMin;
}
PositionSimulationResult PositionSimulation::calculate(const Eigen::Vector3f &attachmentAtomPos, const std::vector<Eigen::Vector4f> &xyzW)
{
	Eigen::Vector4f v(attachmentAtomPos(0),attachmentAtomPos(1),attachmentAtomPos(2),0.0f);
	int atom_i=0;
	for(unsigned i=0; i<xyzW.size(); i++)
	{
		v[3]=xyzW[i][3];
		if( (v-xyzW.at(i)).squaredNorm()<0.01f )
		{
			atom_i=i;
			break;
		}
	}
	return calculate(atom_i,xyzW);
}

PositionSimulationResult PositionSimulationAV3::calculate(unsigned atom_i, const std::vector<Eigen::Vector4f> &xyzW)
{

	const float vdWRMax=2.0;
	const int linknodes=3;

	return PositionSimulationResult(
				calculate3R(static_cast<float>(linkerLength),
				static_cast<float>(linkerWidth),
				static_cast<float>(radius[0]),
				static_cast<float>(radius[1]),
				static_cast<float>(radius[2]),atom_i,
				static_cast<float>(gridResolution),
				vdWRMax,static_cast<float>(allowedSphereRadius),
				linknodes, xyzW));
}

