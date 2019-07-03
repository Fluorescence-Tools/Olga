#ifndef MOLECULARSYSTEMDOMAIN_H
#define MOLECULARSYSTEMDOMAIN_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonObject>


class MolecularSystemDomain
{
public:
	MolecularSystemDomain(const std::string &name);
	size_t numPoints() const;
	// Each string defines a set of atoms.
	// Center Of Mass of these atoms is taken as a reference for
	// determination of orientation of the domain Coordinates of respective
	// selection's COM in LOCAL coordinate system
	std::vector<std::pair<QString, Eigen::Vector3d>> comSellPos;
	QString name;
	// QString selection;
};

#endif // MOLECULARSYSTEMDOMAIN_H
