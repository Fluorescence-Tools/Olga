#include "Position.h"
#include "Distance.h"

#include <map>
#include <iostream>
#include <string>
#include <future>

#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>
Position::Position()
{
	_simulation=0;
	_residueSeqNumber=0;
}

Position::~Position()
{
	delete _simulation;
}

Position::Position(const Position &other):_name(other.name()),_chainIdentifier(other._chainIdentifier),
	_residueSeqNumber(other.residueSeqNumber()),_residueName(other.residueName()),
	_atomName(other.atomName()),_simulationType(other.simulationType())
{
	_simulation=0;
	if(other._simulation)
	{
		_simulation=other._simulation->Clone();
	}
}

Position& Position::operator=(const Position &other)
{
	_name=other.name();
	_chainIdentifier=other.chainIdentifier();
	_residueSeqNumber=other.residueSeqNumber();
	_residueName=other.residueName();
	_atomName=other.atomName();
	if(other._simulation)
	{
		_simulation=other._simulation->Clone();
	}
	return *this;
}

Position::Position(Position&& o):_name(std::move(o._name)),
	_chainIdentifier(std::move(o._chainIdentifier)),
	_residueSeqNumber(o._residueSeqNumber),
	_residueName(std::move(o._residueName)),
	_atomName(std::move(o._atomName)),
	_simulationType(std::move(o._simulationType))
{
	_simulation=o._simulation;
	o._simulation=0;
}

Position& Position::operator=(Position&& o)
{
	_name=std::move(o._name);
	_chainIdentifier=std::move(o._chainIdentifier);
	_residueSeqNumber=o._residueSeqNumber;
	_residueName=std::move(o._residueName);
	_atomName=std::move(o._atomName);
	_simulationType=std::move(o._simulationType);
	_simulation=o._simulation;
	o._simulation=0;
	return *this;
}

Position::Position(const QJsonObject &positionJson, const std::string& name)
{
	_simulation=0;
	load(positionJson, name);
}
QMap<QString,double> loadvdWRadii(const QString& fileName)
{
	QMap<QString,double> map={{"H",0.1},
				  {"C",0.17},
				  {"N",0.1625},
				  {"O",0.149},
				  {"S",0.1782},
				  {"P",0.1}};
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
		return map;
	}
	QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
	if(doc.isNull()) {
		return map;
	}
	QVariantMap varMap=doc.toVariant().toMap();
	using VarMapCI=QVariantMap::const_iterator;
	for (VarMapCI i = varMap.begin(); i != varMap.end(); ++i)
	{
		map[i.key()]=i.value().toDouble();
	}
	return map;
}
float pterosVDW(const pteros::System &system, int i)
{
	//TODO: This is a hack. One shoul define a corresponding function in pteros::System
	static QMap<QString,double> vdWRMap=loadvdWRadii("vdWRadii.json");
	return vdWRMap.value(QString::fromStdString(system.Atom_data(i).name),0.17);
}

std::vector<Eigen::Vector4f> coordsVdW(const pteros::System &system)
{
	//iterate over atoms and fill x,y,z,vdw
	int nAtoms=system.num_atoms();
	std::vector<Eigen::Vector4f> xyzw;
	xyzw.reserve(nAtoms);

	//Fill coordinates
	const pteros::Frame &frame=system.Frame_data(0);
	for (int i=0; i<nAtoms; i++)
	{
		xyzw.emplace_back(frame.coord.at(i)[0]*10.0f,frame.coord.at(i)[1]*10.0f,
				frame.coord.at(i)[2]*10.0f,pterosVDW(system,i)*10.0f);
	}
	return xyzw;
}

PositionSimulationResult Position::calculate(const pteros::System &system) const
{
	std::vector<Eigen::Vector4f> xyzW=coordsVdW(system);
	Eigen::Vector3f refPos=atomXYZ(system);
	return calculate(refPos,xyzW);
}

QJsonObject Position::jsonObject() const
{
	QJsonObject position;
	if(_simulation)
	{
		position=_simulation->jsonObject();
	}
	//position.insert("position_name",QString::fromStdString(_name));
	position.insert("chain_identifier",QString::fromStdString(_chainIdentifier));
	position.insert("residue_seq_number",static_cast<int>(_residueSeqNumber));
	position.insert("residue_name",QString::fromStdString(_residueName));
	position.insert("atom_name",QString::fromStdString(_atomName));
	position.insert("simulation_type",QString::fromStdString(_simulationType));

	return position;
}

bool Position::load(const QJsonObject &positionJson, const std::string& name)
{
	//_name=positionJson.value("position_name").toString().toStdString();
	_name=name;
	_chainIdentifier=positionJson.value("chain_identifier").toString().toStdString();
	_residueSeqNumber=positionJson.value("residue_seq_number").toVariant().toInt();
	_residueName=positionJson.value("residue_name").toString().toStdString();
	_atomName=positionJson.value("atom_name").toString().toStdString();
	_simulationType=positionJson.value("simulation_type").toString().toStdString();
	delete _simulation;
	_simulation=PositionSimulation::create(positionJson);
	return true;
}

const std::string &Position::name() const
{
	return _name;
}

void Position::setName(const std::string &name)
{
	_name=name;
}

std::string Position::chainIdentifier() const
{
	return _chainIdentifier;
}
unsigned Position::residueSeqNumber() const
{
	return _residueSeqNumber;
}

void Position::setResidueSeqNumber(const unsigned &residueSeqNumber)
{
	_residueSeqNumber = residueSeqNumber;
}
std::string Position::residueName() const
{
	return _residueName;
}

void Position::setResidueName(const std::string &residueName)
{
	_residueName = residueName;
}
std::string Position::atomName() const
{
	return _atomName;
}

void Position::setAtomName(const std::string &atomName)
{
	_atomName = atomName;
}
std::string Position::simulationType() const
{
	return _simulationType;
}

void Position::setSimulationType(const std::string &simulationType)
{
	_simulationType = simulationType;
}

std::vector<Position> Position::fromLegacy(const std::string &labelingFileName, const std::string &pdbFileName)
{
	std::vector<Position> positions;
	std::ifstream infile;
	infile.open(labelingFileName.c_str(), std::ifstream::in);
	if(!infile.is_open())
	{
		return std::vector<Position>();
	}

	std::string str;
	while(getline(infile,str))
	{
		if(str.size()==0 )
		{
			continue;
		}
		if(str.at(0)=='#') //comment
		{
			continue;
		}
		Position p;
		p.setFromLegacy(str,pdbFileName);
		positions.push_back(std::move(p));
	}
	return positions;
}

QJsonObject Position::jsonObjects(const std::vector<std::shared_ptr<Position>> &arr)
{
	QJsonObject positions;
	for(const auto& position:arr)
	{
		positions.insert(QString::fromStdString(position->name()),position->jsonObject());
	}
	return positions;
}

Eigen::Vector3f Position::atomXYZ(const pteros::System &system) const
{
	std::string selectionExpression;
	if(_chainIdentifier!=""){
		selectionExpression+="chain "+_chainIdentifier+" and ";
	}
	selectionExpression+="resid "+std::to_string(_residueSeqNumber)+" and ";
	if(_residueName!="")
	{
		selectionExpression+="resname "+_residueName+" and ";
	}
	selectionExpression+="name "+_atomName+" ";
	pteros::Selection select;
	try
	{
		select.modify(system,selectionExpression);
	}
	catch(const pteros::Pteros_error &err)
	{
		std::cerr<<err.what()<<std::endl;
	}

	int selectedCount=select.size();

	if(selectedCount!=1)
	{
		std::cerr <<"Position \""<< _name <<
			    "\". Attachment atom could not be selected. Specified selection defines "<<
			    std::to_string(selectedCount)<<
			    " atoms instead of one: " <<
			    selectionExpression << std::endl;
		const double nan=std::numeric_limits<float>::quiet_NaN();
		return Eigen::Vector3f(nan,nan,nan);
	}
	return select.XYZ(0)*10.0f;
}

PositionSimulationResult Position::calculate(const Eigen::Vector3f& attachmentAtomPos,
					     const std::vector<Eigen::Vector4f>& store) const
{
	return _simulation->calculate(attachmentAtomPos,store);
}

void Position::setFromLegacy(const std::string &entry, const std::string &pdbFileName)
{
	std::istringstream iss(entry);
	std::string buf;
	iss >> _name >> buf >> buf >> _simulationType;
	delete _simulation;
	_simulation=PositionSimulation::create(_simulationType);
	if(!_simulation)
	{
		return;
	}
	int pdbId=_simulation->loadLegacy(iss);
	QFile data(QString::fromStdString(pdbFileName));
	if (data.open(QFile::ReadOnly)) {
		QTextStream in(&data);
		QString str(in.readAll());
		QString selection=QString("[\n^]ATOM *%1").arg(pdbId);
		int pos=str.indexOf(QRegularExpression(selection));
		str=str.mid(pos);
		QTextStream stream(&str);
		QString tmp;
		stream>>tmp>>tmp>>tmp;
		_atomName=tmp.toStdString();
		stream>>tmp;
		_residueName=tmp.toStdString();
		stream>>tmp;
		bool ok;
		_residueSeqNumber=tmp.toInt(&ok);
		if(!ok)
		{
			_chainIdentifier=tmp.toStdString();
			stream>>_residueSeqNumber;
		}
	}
}

void Position::setChainIdentifier(const std::string &chainIdentifier)
{
	_chainIdentifier = chainIdentifier;
}

