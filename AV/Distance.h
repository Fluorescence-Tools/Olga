#ifndef DISTANCE_H
#define DISTANCE_H

#include <QJsonObject>
#include "Position.h"

class Distance
{
public:
    Distance();
    Distance(const QJsonObject& distanceJson, const std::string& name);
    QJsonObject jsonObject() const;
    bool load(const QJsonObject& distanceJson, const std::string& name);

    std::string name() const;
    std::string type() const;
    std::string position1() const;
    std::string position2() const;
    double modelDistance(const PositionSimulationResult& pos1, const PositionSimulationResult& pos2) const;
    double errNeg() const;
    double errPos() const;
    double err(double modelDist) const;
    double R0() const;
    double distance() const;
    double chi2contribution(const PositionSimulationResult& pos1, const PositionSimulationResult& pos2) const;

    void setName(const std::string &name);
    void setR0(double R0);
    void setType(const std::string &type);
    void setPosition1(const std::string &position1);
    void setPosition2(const std::string &position2);
    void setDistance(double distance);
    void setErrNeg(double errNeg);
    void setErrPos(double errPos);

    static std::vector<Distance> fromLegacy(const std::string& distanceFileName);
    static QJsonObject jsonObjects(const std::vector<std::shared_ptr<Distance> > &arr);

private:
    void setFromLegacy(const std::string &entry, const std::string &type);
private:
    std::string _name;
    std::string _type;
    std::string _position1;
    std::string _position2;
    double _distance;
    double _errNeg;
    double _errPos;
    double _R0;
};

#endif // DISTANCE_H
