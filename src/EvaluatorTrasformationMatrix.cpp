#include <iostream>

#include <Eigen/Core>
#include <Eigen/Dense>

#include "EvaluatorTrasformationMatrix.h"
#include "CalcResult.h"


EvaluatorTrasformationMatrix::EvaluatorTrasformationMatrix(
        const TaskStorage &storage, const std::string &name)
    : AbstractEvaluator(storage), _name(name)
{
	comSellPos.resize(3,
	                  std::make_pair("", Eigen::Vector3d(0.0, 0.0, 0.0)));
}

AbstractEvaluator::Task
EvaluatorTrasformationMatrix::makeTask(const FrameDescriptor &frame) const
        noexcept
{
	auto sysTask = getSysTask(frame);
	return sysTask
	        .then([this](pteros::System system) {
		        return calculate(system);
	        })
	        .share();
}

AbstractEvaluator::Setting EvaluatorTrasformationMatrix::setting(int row) const
{
	if (row == 0) {
		return Setting{"num_reference_points", numPoints()};
	} else {
		int i = (row - 1) / 2;
		if (row % 2 == 1) {
			const QString &pname = QString("selection_%1").arg(i);
			const QString &selection =
			        QString::fromStdString(comSellPos[i].first);
			return Setting{pname, selection};
		} else {
			const QString &pname =
			        QString("local_position_%1").arg(i);
			const Eigen::Vector3d &evec = comSellPos[i].second;
			Eigen::Vector3d qvec(evec[0], evec[1], evec[2]);
			return Setting{pname, QVariant::fromValue(qvec)};
		}
	}
}

void EvaluatorTrasformationMatrix::setSetting(int row, const QVariant &val)
{
	if (row == 0) {
		int newPointsCount = val.toInt();
		if (newPointsCount < 3) {
			return;
		} else {
			comSellPos.resize(
			        newPointsCount,
			        std::make_pair("",
			                       Eigen::Vector3d(0.0, 0.0, 0.0)));
		}
	} else {
		int i = (row - 1) / 2;
		if (row % 2 == 1) {
			comSellPos[i].first = val.toString().toStdString();
		} else {
			const Eigen::Vector3d &qvec =
			        val.value<Eigen::Vector3d>();
			Eigen::Vector3d &vec = comSellPos[i].second;
			for (int j : {0, 1, 2}) {
				vec[j] = qvec[j];
			}
		}
	}
}

std::shared_ptr<AbstractCalcResult>
EvaluatorTrasformationMatrix::calculate(const pteros::System &system) const
{
	// auto system=getSystem(desc);
	Eigen::Matrix4d matrix;

	using Eigen::Dynamic;
	Eigen::Matrix<double, 3, Dynamic> positionGlobalCS(3, numPoints());
	Eigen::Matrix<double, 3, Dynamic> positionLocalCS(3, numPoints());
	for (int i = 0; i < numPoints(); i++) {
		pteros::Selection select;
		try {
			select.modify(system, comSellPos[i].first);
		} catch (const pteros::Pteros_error &err) {
			std::cerr << err.what() << std::endl;
		}

		if (select.get_index().size() != 1) {
			std::cerr << std::endl;
			std::cerr
			        << "Specified selection could not be mapped correctly: "
			        << comSellPos[i].first << std::endl;
			matrix = Eigen::Matrix4d().setConstant(
			        std::numeric_limits<double>::quiet_NaN());
			return std::make_shared<CalcResult<Eigen::Matrix4d>>(
			        std::move(matrix));
		}

		positionGlobalCS.col(i) = select.xyz(0).cast<double>() * 10.0;
		positionLocalCS.col(i) = comSellPos[i].second;
	}
	matrix = Eigen::umeyama(positionLocalCS, positionGlobalCS, false);
	return std::make_shared<CalcResult<Eigen::Matrix4d>>(std::move(matrix));
}
