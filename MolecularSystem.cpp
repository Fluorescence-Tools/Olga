#include <iostream>

#include <QTextStream>
#include <QFile>
#include <QFileInfo>

#include <BALL/FORMAT/PDBFile.h>
#include <BALL/KERNEL/system.h>
#include <BALL/STRUCTURE/geometricTransformations.h>
#include <BALL/STRUCTURE/geometricProperties.h>
#include <BALL/FORMAT/molFileFactory.h>
#include <BALL/FORMAT/genericMolFile.h>
#include <BALL/KERNEL/selector.h>
#include <BALL/STRUCTURE/defaultProcessors.h>

#include "MolecularSystem.h"
#include "Distance.h"

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
    if(system.countMolecules()==0)
    {
        load();
    }
    return transformationMatrix(_domain).block<3,3>(0,0).eulerAngles(2,1,2)*(180.0/3.141592653589793);
}

Eigen::Vector3d MolecularSystem::eulerAngles(const MolecularSystemDomain &_domain, const MolecularSystemDomain &_referenceDomain) const
{
    if(system.countMolecules()==0)
    {
        load();
    }
    const Eigen::Matrix3d& Rd=transformationMatrix(_domain).block<3,3>(0,0);
    const Eigen::Matrix3d& Rrd=transformationMatrix(_referenceDomain).block<3,3>(0,0);
    /*std::cout<<"Domain: "<<_domain.name.toStdString()<<"; Ref.Domain: "<<_referenceDomain.name.toStdString()<<'\n';
    std::cout<<"Rd:\n"<<Rd<<'\n';
    std::cout<<"Rrd:\n"<<Rrd<<'\n';
    std::cout<<"Rrd.transpose()*Rd:\n"<<(Rrd.transpose()*Rd)<<'\n';
    std::cout<<"angles: "<<((Rrd.transpose()*Rd).eulerAngles(2,1,2)*(180.0/3.141592653589793)).transpose()<<std::endl;*/

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
    if(system.countMolecules()==0)
    {
        load();
    }
    size_t nPoints=std::min(_domain.COMselections.size(),_domain.COMpositionLocalCS.size());
    using Eigen::Dynamic;
    Eigen::Matrix<double,3,Dynamic> positionGlobalCS(3,nPoints);
    Eigen::Matrix<double,3,Dynamic> positionLocalCS(3,nPoints);
    for(size_t i=0; i<nPoints; i++)
    {
        BALL::Selector select(_domain.COMselections.at(i).toLocal8Bit().data());
        system.apply(select);
        std::list<BALL::Atom*> selected_atoms = select.getSelectedAtoms();
        if(selected_atoms.size()!=1)
        {
            std::cerr << name() << ": specified selection could not be mapped: " << _domain.COMselections.at(i).toLocal8Bit().data() << std::endl;
            return Eigen::Matrix4d().setConstant(std::numeric_limits<double>::quiet_NaN());
        }

        BALL::TVector3<float> &frontAtomPos=selected_atoms.front()->getPosition();
        system.deselect();
        positionGlobalCS.col(i)=Vector3d(frontAtomPos.x,frontAtomPos.y,frontAtomPos.z);
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
    if(system.countMolecules()==0)
    {
        load();
    }
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
    if(system.countMolecules()==0)
    {
        load();
    }
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
    if(system.countMolecules()==0)
    {
        load();
    }
    BALL::PDBFile outfile(_filename.toLocal8Bit().data(), std::ios::out);
    outfile << system;
    outfile.close();
    unload();
    return true;
}

void MolecularSystem::unload() const
{
    system.clear();
}

double MolecularSystem::chi2(const std::vector<Position> &positions, const std::vector<Distance> &distances) const
{
    if(system.countMolecules()==0)
    {
        load();
    }
    try{
        BALL::AssignRadiusProcessor arp("radii/amber94.siz");
        system.apply(arp);
    }
    catch(...)
    {
        std::cerr<<"Could not assign vdW radii. Probably, radii/amber94.siz file is not accessible."<<std::endl;
    }
    return Position::chi2(system,positions,distances);
}
bool MolecularSystem::load() const
{
    QString suffix=_filename.section('.',-1).toLower();
    if(suffix=="pml")
    {
        return loadPml();
    }
    return loadSystem(_filename, system);

}
bool MolecularSystem::load(const QString &filename)
{
    _name=QFileInfo(filename).fileName();
    _filename=filename;
    return load();
}
bool MolecularSystem::loadSystem(const QString& _filename,BALL::System &system)
{
    try
    {
        BALL::GenericMolFile* infile = BALL::MolFileFactory::open( _filename.toLocal8Bit().data());
        if (!infile)
        {
          std::cerr << _filename.toLocal8Bit().data()<< ": could not determine filetype, aborting" << std::endl;
          return false;
        }

        if (!*infile)
        {
          std::cerr <<_filename.toLocal8Bit().data()<< ": invalid file, aborting" << std::endl;
          return false;
        }

        *infile >> system;

        // Important: Cleanup
        infile->close();
        delete infile;
        return true;
    }
    catch(...)
    {
        std::cerr <<_filename.toLocal8Bit().data()<< ": could not open file" << std::endl;
        return false;
    }
}

bool MolecularSystem::loadPml() const
{
    std::map<QString,BALL::System> bodies;//name,molecule
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
                std::cerr << _filename.toLocal8Bit().data() <<": molecule has not been loaded ("<<name.toLocal8Bit().data()<<"), can not rotate"<<std::endl;
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

