#include "Distance.h"

Distance::Distance(const QJsonObject &distanceJson, const std::string &name)
{
	load(distanceJson, name);
}

QJsonObject Distance::jsonObject() const
{
	QJsonObject distance;
	distance.insert("position1_name", QString::fromStdString(_position1));
	distance.insert("position2_name", QString::fromStdString(_position2));
	distance.insert("distance", _distance);
	distance.insert("error_neg", _errNeg);
	distance.insert("error_pos", _errPos);
	distance.insert("distance_type", QString::fromStdString(_type));
	distance.insert("Forster_radius", _R0);
	return distance;
}

bool Distance::load(const QJsonObject &distanceJson, const std::string &name)
{
	_name = name;
	_type = distanceJson.value("distance_type").toString().toStdString();
	_position1 =
		distanceJson.value("position1_name").toString().toStdString();
	_position2 =
		distanceJson.value("position1_name").toString().toStdString();
	_distance = distanceJson.value("distance").toDouble();
	_errNeg = distanceJson.value("error_neg").toDouble();
	_errPos = distanceJson.value("error_pos").toDouble();
	_R0 = distanceJson.value("Forster_radius").toDouble();
	return true;
}

std::string Distance::type() const
{
	return _type;
}
std::string Distance::position1() const
{
	return _position1;
}
std::string Distance::position2() const
{
	return _position2;
}

double Distance::modelDistance(const PositionSimulationResult &pos1,
			       const PositionSimulationResult &pos2) const
{
	return pos1.modelDistance(pos2, _type, _R0);
}

double Distance::RmpFromModelDistance(const PositionSimulationResult &av1,
				      PositionSimulationResult av2,
				      const double targetModelDist,
				      const double accuracy) const
{
	Eigen::Vector3f av1Mp = av1.meanPosition();
	Eigen::Vector3f av2Mp = av2.meanPosition();
	Eigen::Vector3f direction = (av2Mp - av1Mp).normalized();
	double Rmod = modelDistance(av1, av2);
	double diff = targetModelDist - Rmod;
	double prevDiff = diff;
	for (; std::fabs(diff) > accuracy;
	     Rmod = modelDistance(av1, av2), diff = targetModelDist - Rmod) {
		if (std::fabs(prevDiff) < std::fabs(diff)) {
			break;
		}
		prevDiff = diff;
		av2.translate(direction * diff);
	}
	/*qDebug()<<"Rmp: "<<av1.Rmp(av2)<<" Rmodel:"<<modelDistance(av1,av2)<<
		  " Rda:"<<av1.Rda(av2)<<" RdaE: "<<av1.Rdae(av2,52.0);*/
	return av1.Rmp(av2);
}
double Distance::errNeg() const
{
	return _errNeg;
}
double Distance::errPos() const
{
	return _errPos;
}

double Distance::err(double modelDist) const
{
	return modelDist < _distance ? _errNeg : _errPos;
}
double Distance::R0() const
{
	return _R0;
}
double Distance::distance() const
{
	return _distance;
}

double Distance::chi2contribution(const PositionSimulationResult &pos1,
				  const PositionSimulationResult &pos2) const
{
	double modelDist = modelDistance(pos1, pos2);
	double contrib = (modelDist - distance()) / err(modelDist);
	return contrib * contrib;
}

std::vector<Distance> Distance::fromLegacy(const std::string &distanceFileName)
{
	std::vector<Distance> distances;
	std::ifstream infile;
	infile.open(distanceFileName.c_str(), std::ifstream::in);
	if (!infile.is_open()) {
		return std::vector<Distance>();
	}

	std::string type;
	infile >> type;
	std::string str;
	while (getline(infile, str)) {
		if (str.size() == 0) {
			continue;
		}
		if (str.at(0) == '#') // comment
		{
			continue;
		}
		Distance d;
		d.setFromLegacy(str, type);
		distances.push_back(std::move(d));
	}
	return distances;
}

QJsonObject
Distance::jsonObjects(const std::vector<std::shared_ptr<Distance>> &arr)
{
	QJsonObject distances;
	for (const auto &distance : arr) {
		distances.insert(QString::fromStdString(distance->name()),
				 distance->jsonObject());
	}
	return distances;
}

void Distance::setFromLegacy(const std::string &entry, const std::string &type)
{
	_type = type;
	std::istringstream iss(entry);
	std::string buf;
	iss >> _position1 >> _position2 >> _distance >> _errPos >> _errNeg;
	_name = _position1 + "_" + _position2;
}
std::string Distance::name() const
{
	return _name;
}
void Distance::setName(const std::string &name)
{
	_name = name;
}
void Distance::setErrPos(double errPos)
{
	_errPos = errPos;
}
void Distance::setErrNeg(double errNeg)
{
	_errNeg = errNeg;
}

void Distance::setDistance(double distance)
{
	_distance = distance;
}

void Distance::setPosition2(const std::string &position2)
{
	_position2 = position2;
}

void Distance::setPosition1(const std::string &position1)
{
	_position1 = position1;
}

void Distance::setType(const std::string &type)
{
	_type = type;
}

void Distance::setR0(double R0)
{
	_R0 = R0;
}
