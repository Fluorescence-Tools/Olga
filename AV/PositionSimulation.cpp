#include "PositionSimulation.h"
#include "av_routines.h"
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
	case Position::SimulationType::av1:
		return new PositionSimulationAV1;
	case Position::SimulationType::av3:
		return new PositionSimulationAV3;
	case Position::SimulationType::atom:
		return new PositionSimulationAV1;
	}
	return new PositionSimulationAV1;
}
PositionSimulation* PositionSimulation::create(const QVariantMap &settings)
{
	Position::SimulationType type=settings.value("simulation_type").value<Position::SimulationType>();
	PositionSimulation* pos=create(type);
	pos->load(settings);
	return pos;
}

bool PositionSimulationAV3::load(const QVariantMap& settings)
{
	gridResolution=settings.value("simulation_grid_resolution",0.4).toDouble();
	linkerLength=settings.value("linker_length").toDouble();
	linkerWidth=settings.value("linker_width").toDouble();
	radius[0]=settings.value("radius1").toDouble();
	radius[1]=settings.value("radius2").toDouble();
	radius[2]=settings.value("radius3").toDouble();
	allowedSphereRadius=settings.value("allowed_sphere_radius",-1.0).toDouble();
	allowedSphereRadiusMin=settings.value("allowed_sphere_radius_min",0.5).toDouble();
	allowedSphereRadiusMax=settings.value("allowed_sphere_radius_max",2.0).toDouble();
	return true;
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
	}
}

PositionSimulationResult
PositionSimulation::calculate(const Eigen::Vector3f &attachmentAtomPos,
			      const std::vector<Eigen::Vector4f> &xyzW)
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

PositionSimulationResult
PositionSimulationAV3::calculate(unsigned atom_i,
				 const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector3f> res;
	double as=getAllowedSphereRadius(atom_i,xyzW,res);
	if(as!=allowedSphereRadius) {
		std::cout<<"allowed sphere radius determined: "
			   +std::to_string(as)+"A\n"<<std::flush;
	}
	return PositionSimulationResult(std::move(res));
}

double PositionSimulationAV3::
getAllowedSphereRadius(unsigned atom_i, const std::vector<Eigen::Vector4f> &xyzW,
		       std::vector<Eigen::Vector3f> &res) const
{
	res.clear();
	if(allowedSphereRadius>0.0f) {
		res=std::move(calculate3R(linkerLength, linkerWidth, radius[0],
			      radius[1],radius[2], atom_i,gridResolution, vdWRMax,
				allowedSphereRadius, linknodes, xyzW));
	}
	if(res.size()>0) {
		return allowedSphereRadius;
	}

	const double minRadius=*std::min_element(radius,radius+3);
	const double dAs=(allowedSphereRadiusMax-allowedSphereRadiusMin)/5.0;
	size_t prevResSize=0;
	for(double as=allowedSphereRadiusMin; as<=allowedSphereRadiusMax;as+=dAs)
	{
		res.clear();
		res=std::move(calculate1R(linkerLength, linkerWidth, minRadius,
					  atom_i,gridResolution, vdWRMax,
					  as, linknodes, xyzW));
		//check if AV size is stable for two different allowed spheres
		if(res.size()/2<prevResSize) {
			res=calculate3R(static_cast<float>(linkerLength),
					static_cast<float>(linkerWidth),
					static_cast<float>(radius[0]),
					static_cast<float>(radius[1]),
					static_cast<float>(radius[2]),atom_i,
					static_cast<float>(gridResolution),
					vdWRMax,
					static_cast<float>(as),
					linknodes, xyzW);
			if(!res.empty()) {
				return as;
			}
		}
		prevResSize=res.size();
	}
	res.clear();
	return -1.0;
}

bool PositionSimulationAV1::load(const QVariantMap& settings)
{
	gridResolution=settings.value("simulation_grid_resolution",0.4).toDouble();
	linkerLength=settings.value("linker_length").toDouble();
	linkerWidth=settings.value("linker_width").toDouble();
	radius=settings.value("radius1").toDouble();
	allowedSphereRadius=settings.value("allowed_sphere_radius",0.5).toDouble();
	allowedSphereRadiusMin=settings.value("allowed_sphere_radius_min",0.5).toDouble();
	allowedSphereRadiusMax=settings.value("allowed_sphere_radius_max",2.0).toDouble();
	return true;
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
	}
}

PositionSimulationResult PositionSimulationAV1::calculate(unsigned atom_i, const std::vector<Eigen::Vector4f> &xyzW)
{
	std::vector<Eigen::Vector3f> res;
	double as=getAllowedSphereRadius(atom_i,xyzW,res);
	if(as!=allowedSphereRadius) {
		std::cout<<"allowed sphere radius determined: "
			   +std::to_string(as)+"A\n"<<std::flush;
	}
	return PositionSimulationResult(std::move(res));
}

double PositionSimulationAV1::
getAllowedSphereRadius(unsigned atom_i, const std::vector<Eigen::Vector4f> &xyzW,
		       std::vector<Eigen::Vector3f> &res) const
{
	res.clear();
	if(allowedSphereRadius>0.0f) {
		res=std::move(calculate1R(linkerLength, linkerWidth, radius,
					  atom_i,gridResolution, vdWRMax,
					  allowedSphereRadius, linknodes, xyzW));
	}
	if(res.size()>0) {
		return allowedSphereRadius;
	}

	const double dAs=(allowedSphereRadiusMax-allowedSphereRadiusMin)/5.0;
	size_t prevResSize=0;
	for(double as=allowedSphereRadiusMin; as<=allowedSphereRadiusMax;as+=dAs)
	{
		res.clear();
		res=std::move(calculate1R(linkerLength, linkerWidth, radius,
					  atom_i,gridResolution, vdWRMax,
					  as, linknodes, xyzW));
		//check if AV size is stable for two different allowed spheres
		if(res.size()/2<prevResSize) {
			return as;
		}
		prevResSize=res.size();
	}
	res.clear();
	return -1;
}

/*
PositionSimulation* PositionSimulation::create(const QJsonObject &positionJson)
{
	std::string type=positionJson.value("simulation_type").toString().toStdString();
	PositionSimulation* pos=create(type);
	pos->load(positionJson);
	return pos;
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
QJsonObject PositionSimulationAV3::jsonObject() const
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

PositionSimulationAV1::PositionSimulationAV1(const QJsonObject &positionJson)
{
	load(positionJson);
}

bool PositionSimulationAV1::load(const QJsonObject &positionJson)
{
	gridResolution=positionJson.value("simulation_grid_resolution").toDouble(0.4);
	linkerLength=positionJson.value("linker_length").toDouble(0.0);
	linkerWidth=positionJson.value("linker_width").toDouble(0.0);
	radius=positionJson.value("radius1").toDouble(0.0);
	allowedSphereRadius=positionJson.value("allowed_sphere_radius").toDouble(0.5);
	allowedSphereRadiusMin=positionJson.value("allowed_sphere_radius_min").toDouble(0.5);
	allowedSphereRadiusMax=positionJson.value("allowed_sphere_radius_max").toDouble(2.0);
	return true;
}

QJsonObject PositionSimulationAV1::jsonObject() const
{
	QJsonObject position;
	position.insert("simulation_grid_resolution",gridResolution);
	position.insert("linker_length",linkerLength);
	position.insert("linker_width",linkerWidth);
	position.insert("radius1",radius);
	position.insert("allowed_sphere_radius",allowedSphereRadius);
	position.insert("allowed_sphere_radius_min",allowedSphereRadiusMin);
	position.insert("allowed_sphere_radius_max",allowedSphereRadiusMax);
	return position;
}

*/
