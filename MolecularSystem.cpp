#include <iostream>

#include <QTextStream>
#include <QFile>
#include <QFileInfo>

#include "MolecularSystem.h"
#include "AV/Distance.h"

using Eigen::Vector3d;
using Eigen::Matrix3d;

void ignore(QTextStream& ss, char delim)
{
	char c;
	do
	{
		ss>>c;
	}
	while (c!=delim);
}

MolecularSystem::MolecularSystem()
{
}

Eigen::Vector3d MolecularSystem::eulerAngles(const MolecularSystemDomain &_domain) const
{
	return transformationMatrix(_domain).block<3,3>(0,0).
			eulerAngles(2,1,2)*(180.0/3.141592653589793);
}

Eigen::Vector3d MolecularSystem::
eulerAngles(const MolecularSystemDomain &_domain, const MolecularSystemDomain &_referenceDomain) const
{
	const Eigen::Matrix3d& Rd=transformationMatrix(_domain).block<3,3>(0,0);
	const Eigen::Matrix3d& Rrd=transformationMatrix(_referenceDomain).block<3,3>(0,0);

	Eigen::Vector3d angles=(Rrd.transpose()*Rd).eulerAngles(2,1,2)*(180.0/3.141592653589793);
	if(angles(1)<0.0)
	{
		angles={angles(0)+180.0,-1.0*angles(1),angles(2)+180.0};
		angles[0]-=angles[0]>180.0?360.0:0.0;
		angles[2]-=angles[2]>180.0?360.0:0.0;
	}
	return angles;
}

Eigen::Matrix4d MolecularSystem::transformationMatrix(const MolecularSystemDomain &_domain) const
{
	size_t nPoints=std::min(_domain.COMselections.size(),_domain.COMpositionLocalCS.size());
	using Eigen::Dynamic;
	Eigen::Matrix<double,3,Dynamic> positionGlobalCS(3,nPoints);
	Eigen::Matrix<double,3,Dynamic> positionLocalCS(3,nPoints);
	for(size_t i=0; i<nPoints; i++)
	{
		pteros::Selection select;
		try
		{
			select.modify(system,_domain.COMselections.at(i).toLocal8Bit().data());
		}
		catch (const pteros::Pteros_error &err)
		{
			std::cerr<<err.what()<<std::endl;
		}

		if(select.get_index().size()!=1)
		{
			std::cerr << name() <<
				     ": specified selection could not be mapped correctly: " <<
				     _domain.COMselections.at(i).toLocal8Bit().data() << std::endl;
			return Eigen::Matrix4d().setConstant(std::numeric_limits<double>::quiet_NaN());
		}

		positionGlobalCS.col(i)=select.XYZ(0).cast<double>()*10.0;
		positionLocalCS.col(i)=_domain.COMpositionLocalCS.at(i);

	}
	Eigen::Matrix4d R=Eigen::umeyama(positionLocalCS,positionGlobalCS,false);
	return R;
}

Eigen::Vector3d MolecularSystem::pointPosition(const MolecularSystemDomain &_domain, const Eigen::Vector3d &point) const
{
	Eigen::Matrix4d m=transformationMatrix(_domain);
	return (m.block<3,3>(0,0))*point+m.block<3,1>(0,3);
}

double MolecularSystem::maxZ(const MolecularSystemDomain &_domain) const
{
	double maxZ=-std::numeric_limits<double>::infinity();
	for(const Eigen::Vector3d& v:_domain.COMpositionLocalCS)
	{
		const double &z=v[2];
		maxZ=z>maxZ?z:maxZ;
	}
	return maxZ;
}

double MolecularSystem::minZ(const MolecularSystemDomain &_domain) const
{
	double minZ=std::numeric_limits<double>::infinity();
	for(const Eigen::Vector3d& v:_domain.COMpositionLocalCS)
	{
		const double &z=v[2];
		minZ=z<minZ?z:minZ;
	}
	return minZ;
}

bool MolecularSystem::save(const QString &_filename) const
{
	pteros::Selection sel(system,"all");
	sel.write(_filename.toLocal8Bit().data());
	return true;
}

double MolecularSystem::chi2(const std::vector<Position> &positions, const std::vector<Distance> &distances) const
{
	double d=Position::chi2(system,positions,distances);
	if (std::isnan(d))
	{
		std::cerr<<"Some distances could not be calculated for structure "<<
			   this->name()<<std::endl;
	}
	return d;
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

std::vector<Eigen::Vector4f> MolecularSystem::coordsVdW(const pteros::System &system)
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
bool MolecularSystem::load() const
{
	QString suffix=_filename.section('.',-1).toLower();
	/*if(suffix=="pml")
	{
		return loadPml();
	}*/
	return loadSystem(_filename, system);

}
bool MolecularSystem::load(const QString &filename)
{
	_name=QFileInfo(filename).fileName();
	_filename=filename;
	return load();
}
bool MolecularSystem::loadSystem(const QString& _filename,pteros::System &system)
{
	system.clear();
	system.load(_filename.toStdString());
	return true;
}
/*
bool MolecularSystem::loadPml() const
{
	std::map<QString,pteros::System> bodies;//name,molecule
	QFile file(_filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		if(line.at(0)=='#')
		{
			continue;
		}
		QTextStream ss(&line);
		QString word;
		ss>>word;
		if(word=="load")
		{
			QString fileName;
			ss.skipWhiteSpace();
			fileName=ss.readLine();
			QString name=QFileInfo(fileName).fileName();
			QString basename=QFileInfo(fileName).baseName();
			if(!loadSystem(fileName,bodies[basename]))
			{
				std::cerr <<_filename.toLocal8Bit().data()<< ": could not load file: "<< fileName.toLocal8Bit().data() << std::endl;
			}
			continue;
		}

		if(word=="rotate")
		{
			QString name;
			ignore(ss,'[');//skip " ["
			double x,y,z,a, Ox, Oy, Oz;
			ss>>x;
			ignore(ss,' ');
			ss>>y;
			ignore(ss,' ');
			ss>>z;
			ignore(ss,' ');
			ss>>a;
			ignore(ss,' ');
			ss>>name;
			name=name.section(',',0,0);
			auto itBody=bodies.find(name);
			if(itBody==bodies.end())
			{
				std::cerr <<_filename.toLocal8Bit().data() << ": domain "<<name.toLocal8Bit().data()<<" has not been loaded, skipping."<<std::endl;
				continue;
			}

			ignore(ss,'[');
			ss>>Ox;
			ignore(ss,' ');
			ss>>Oy;
			ignore(ss,' ');
			ss>>Oz;

			BALL::Angle angle = BALL::Angle(a/180.0*BALL::Constants::PI, true);
			BALL::Vector3 rotationaxis(x, y, z);
			BALL::Matrix4x4 mat;
			mat.setRotation(angle, rotationaxis);
			BALL::TransformationProcessor transformation(mat);

			BALL::Vector3 origin(-Ox,-Oy,-Oz);
			BALL::TranslationProcessor translation;
			translation.setTranslation(origin);

			if(itBody->second.getMolecule(0)==0)
			{
				std::cerr << _filename.toLocal8Bit().data() <<
					     ": molecule has not been loaded ("<<
					     name.toLocal8Bit().data()<<
					     "), can not rotate"<<std::endl;
				continue;
			}
			itBody->second.getMolecule(0)->apply(translation);
			itBody->second.getMolecule(0)->apply(transformation);
			origin.negate();
			translation.setTranslation(origin);
			itBody->second.getMolecule(0)->apply(translation);

			continue;
		}
		if(word=="translate")
		{
			QString name;
			ignore(ss,'[');//skip " ["
			double x,y,z;
			ss>>x;
			ignore(ss,' ');
			ss>>y;
			ignore(ss,' ');
			ss>>z;
			ignore(ss,' ');
			ss>>name;
			auto itBody=bodies.find(name);
			if(itBody==bodies.end())
			{
				std::cerr << _filename.toLocal8Bit().data() <<": domain "<<name.toLocal8Bit().data()<<" has not been loaded, skipping."<<std::endl;
				continue;
			}
			BALL::TranslationProcessor translation;
			BALL::Vector3 tV(x,y,z);
			translation.setTranslation(tV);
			if(itBody->second.getMolecule(0)==0)
			{
				std::cerr << _filename.toLocal8Bit().data() <<": molecule has not been loaded ("<<name.toLocal8Bit().data()<<"), can not translate"<<std::endl;
				continue;
			}
			itBody->second.getMolecule(0)->apply(translation);
			continue;
		}
	}
	file.close();
	typedef std::map<QString,BALL::System>::iterator it_type;
	for(it_type iterator = bodies.begin(); iterator != bodies.end(); iterator++)
	{
		system.append( *(iterator->second.getMolecule(0)) );
	}
	return true;
}
*/
