#ifndef EVALUATORTRANSORMATIONMATRIX_H
#define EVALUATORTRANSORMATIONMATRIX_H

#include "AbstractEvaluator.h"

class EvaluatorTrasformationMatrix : public AbstractEvaluator
{
private:
	//MolecularSystemDomain _domain;
	std::vector<std::pair<std::string,Eigen::Vector3d>> comSellPos;
	std::string _name;
	int numPoints() const {
		return comSellPos.size();
	}
public:
	EvaluatorTrasformationMatrix(const TaskStorage& storage,
				     const std::string& name);
	virtual Task
		makeTask(const FrameDescriptor &frame) const noexcept;

	virtual std::string name() const
	{
		return _name;
	}
	virtual void setName(const std::string& name){
		_name=name;
	}
	virtual std::string className() const
	{
		return "Coordinate systems";
	}
	virtual int columnCount() const
	{
		return 0;
	}
	virtual std::string columnName(int i) const
	{
		(void)i;
		return "none";
	}
	virtual int settingsCount() const
	{
		return 2*numPoints()+1;
	}
	virtual Setting setting(int row) const
	{
		if(row==0) {
			return Setting{"num_reference_points",numPoints()};
		} else {
			int i=(row-1)/2;
			if(row%2==1) {
				const QString& pname=QString("selection_%1").arg(i);
				const QString& selection=QString::fromStdString(comSellPos[i].first);
				return Setting{pname, selection};
			} else {
				const QString& pname=QString("local_position_%1").arg(i);
				const Eigen::Vector3d& evec=comSellPos[i].second;
				Eigen::Vector3d qvec(evec[0],evec[1],evec[2]);
				return Setting{pname,QVariant::fromValue(qvec)};
			}
		}
	}
	virtual void setSetting(int row, const QVariant& val)
	{
		if(row==0) {
			int newPointsCount=val.toInt();
			if(newPointsCount<3) {
				return;
			} else {
				comSellPos.resize(newPointsCount,
						  std::make_pair("",Eigen::Vector3d(0.0,0.0,0.0)));
			}
		} else {
			int i=(row-1)/2;
			if(row%2==1) {
				comSellPos[i].first=val.toString().toStdString();
			} else {
				const Eigen::Vector3d& qvec=val.value<Eigen::Vector3d>();
				Eigen::Vector3d& vec=comSellPos[i].second;
				for(int j:{0,1,2}) {vec[j]=qvec[j];}
			}
		}
	}


private:
	std::shared_ptr<AbstractCalcResult> calculate(const pteros::System &system) const;
};

#endif // EVALUATORTRANSORMATIONMATRIX_H
