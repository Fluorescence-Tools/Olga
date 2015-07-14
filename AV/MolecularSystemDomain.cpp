#include "MolecularSystemDomain.h"
#include <QJsonArray>

MolecularSystemDomain::MolecularSystemDomain(const std::string &name):
	name(QString::fromStdString(name))
{
	comSellPos.resize(4);
}
/*
MolecularSystemDomain::MolecularSystemDomain(const QJsonObject &obj)
{
    name=obj.value("domain_name").toString();
    selection=obj.value("domain_selection_expression").toString();
    QJsonArray referencePoints=obj.value("reference_points").toArray();
    for(const QJsonValue &val:referencePoints)
    {
	QJsonObject rp=val.toObject();
	double xLocal=rp.value("x_local").toDouble();
	double yLocal=rp.value("y_local").toDouble();
	double zLocal=rp.value("z_local").toDouble();
	COMpositionLocalCS.push_back(Eigen::Vector3d(xLocal,yLocal,zLocal));
	COMselections.push_back(rp.value("com_custom_selection_expression").toString());
    }
}

QJsonObject MolecularSystemDomain::jsonObj() const
{
    QJsonObject obj;

    QJsonArray referencePoints;
    size_t numPoints=std::min(COMselections.size(),COMpositionLocalCS.size());
    for(size_t i=0; i<numPoints;i++)
    {
	QJsonObject point;
	point.insert("x_local",COMpositionLocalCS.at(i)(0));
	point.insert("y_local",COMpositionLocalCS.at(i)(1));
	point.insert("z_local",COMpositionLocalCS.at(i)(2));
	point.insert("com_custom_selection_expression",COMselections.at(i));
	referencePoints.append(point);
    }

    obj.insert("domain_name",name);
    obj.insert("domain_selection_expression",selection);
    obj.insert("reference_points",referencePoints);
    return obj;
}*/

size_t MolecularSystemDomain::numPoints() const
{
    return comSellPos.size();
}
