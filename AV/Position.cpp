#include "Position.h"
#include "Distance.h"
#include "MolecularSystem.h"

#include <map>
#include <iostream>
#include <string>
#include <future>

#include <QJsonObject>
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
}

Position::Position(const QJsonObject &positionJson, const std::string& name)
{
	_simulation=0;
	load(positionJson, name);
}
float pterosVDW(const pteros::System &system, int i)
{
	//TODO: This is a hack. One shoul define a corresponding function in pteros::System
	switch(system.Atom_data(i).name[0]){
	case 'H': return  0.1;
	case 'C': return  0.17;
	case 'N': return  0.1625;
	case 'O': return  0.149; //mean value used
	case 'S': return  0.1782;
	case 'P': return  0.1871;
	default:  return  0.17;
	}
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

PositionSimulationResult Position::calculate(pteros::System &system) const
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
	position.insert("residue_seq_number",(int)_residueSeqNumber);
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
/*
double Position::obsolete_chi2(pteros::System &system, const std::vector<Position> &positions,
		      const std::vector<Distance> &distances)
{

	//arrange positions for fast access
	std::map<std::string,std::vector<Position>::const_iterator> positionsMap;
	for(std::vector<Position>::const_iterator it=positions.begin(); it!=positions.end(); it++ )
	{
		positionsMap[it->name()]=it;
	}

	//prepare memory
	std::vector<Eigen::Vector4f> xyzW=MolecularSystem::coordsVdW(system);

	typedef std::map<std::string,std::future<PositionSimulationResult>> TFutMap;
	TFutMap futures;
	for(const Distance& dist:distances)
	{
		//calculate AVs
		const std::string names[2]={dist.position1(),dist.position2()};
		TFutMap::const_iterator simIt[2];
		int iName;
		for(iName=0; iName<2; iName++)
		{
			simIt[iName]=futures.find(names[iName]);
			if(simIt[iName]==futures.end())
			{
				auto it=positionsMap.find(names[iName]);
				if(it==positionsMap.end())
				{
					std::cerr<<"Position "<<names[iName]<<
						   " not found. Corresponding distances will be ignored."<<std::endl;
					return std::numeric_limits<double>::quiet_NaN();
					break;
				}
				else
				{
					Eigen::Vector3f refPos=it->second->atomXYZ(system);
					futures[names[iName]]=std::async(std::launch::async,&Position::calculate,it->second,refPos,std::cref(xyzW));
					//simulations[names[iName]]=it->second->calculate(pos,store);
					/*threads.push_back(std::thread([=,&simulations,&store](){
			PositionSimulationResult res=std::move(it->second->calculate(pos,store));
			if(!res.empty()){
			    simulations[names[iName]]=std::move(res);
			}
		    }));*//*
				}
			}
		}
	}
	typedef std::map<std::string,PositionSimulationResult> TSimMap;
	TSimMap simulations;
	for(auto& f : futures){
		auto it=simulations.insert(std::make_pair(f.first,f.second.get())).first;

		if(it->second.empty())
		{
			std::cerr<<"AV Simulation failed for "<<f.first<<std::endl;
			return std::numeric_limits<double>::quiet_NaN();
			simulations.erase(it);
		}
	}
	double chi2=0.0;
	for(const Distance& dist:distances)
	{
		//calculate system distances
		const std::string &name1=dist.position1(), &name2=dist.position2();
		TSimMap::const_iterator simIt1=simulations.find(name1), simIt2=simulations.find(name2);
		if(simIt1!=simulations.end() && simIt2!=simulations.end())
		{
			chi2+=dist.chi2contribution(simIt1->second, simIt2->second);
		}
	}
	return chi2;
}
*/
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

Eigen::Vector3f Position::atomXYZ(pteros::System &system) const
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

