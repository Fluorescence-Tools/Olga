#ifndef MOLECULARSYSTEM_H
#define MOLECULARSYSTEM_H

#include <Eigen/Core>
#include <Eigen/Dense>

#include "pteros/pteros.h"

#include "AV/MolecularSystemDomain.h"
#include "AV/Distance.h"
#include "AV/Position.h"



class MolecularSystem
{
public:
	enum EulerAngleId {Alpha, Beta, Gamma};
	MolecularSystem();
	std::string name() const
	{
		return _name.toStdString();
	}
	bool load(const QString &filename);
	//double eulerAngle(const MolecularSystemDomain& _domain, EulerAngleId _angleId) const;
	Eigen::Vector3d eulerAngles(const MolecularSystemDomain& _domain) const;
	Eigen::Vector3d eulerAngles(const MolecularSystemDomain& _domain,
				    const MolecularSystemDomain& _referenceDomain) const;
	Eigen::Matrix4d transformationMatrix(const MolecularSystemDomain &_domain) const;
	Eigen::Vector3d pointPosition(const MolecularSystemDomain& _domain,
				      const Eigen::Vector3d& point) const;
	double maxZ(const MolecularSystemDomain& _domain) const;
	double minZ(const MolecularSystemDomain& _domain) const;
	bool save(const QString& _filename) const;
	double chi2(const std::vector<Position>& positions,
		    const std::vector<Distance>& distances) const;

	static std::vector<Eigen::Vector4f> coordsVdW(const pteros::System &system);

protected:
private:
	bool load() const;
	//bool loadPml() const;
	static bool loadSystem(const QString &_filename, pteros::System &system);

private:
	QString _name;
	QString _filename;

	mutable pteros::System system;
	//    mutable Eigen::Matrix3d R; //rotation matrix cached value
	//    mutable Eigen::Vector3d translation; //translation vector cached value
	//    mutable double alpha, beta, gamma; //Euler angles cached values
	//    mutable MolecularSystemDomain domain; //Domain relevant for cached values
};

#endif // MOLECULARSYSTEM_H
