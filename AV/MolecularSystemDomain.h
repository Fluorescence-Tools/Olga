#ifndef MOLECULARSYSTEMDOMAIN_H
#define MOLECULARSYSTEMDOMAIN_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonObject>


class MolecularSystemDomain
{
public:
    MolecularSystemDomain();
    MolecularSystemDomain(const QJsonObject &obj);
    QJsonObject jsonObj() const;
    size_t numPoints() const;


    //Each string defines a set of atoms. Center Of Mass of these atoms is taken as a reference for determination of orientation of the domain
    std::vector<QString> COMselections;
    //Coordinates of respective selection's COM in LOCAL coordinate system
    std::vector<Eigen::Vector3d> COMpositionLocalCS;
    QString name;
    QString selection;
};

#endif // MOLECULARSYSTEMDOMAIN_H
