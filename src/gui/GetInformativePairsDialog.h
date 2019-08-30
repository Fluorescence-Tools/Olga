#ifndef GETINFORMATIVEPAIRSDIALOG_H
#define GETINFORMATIVEPAIRSDIALOG_H

#include <QDialog>

#include "FrameDescriptor.h"
#include <pteros/pteros.h>
#include <Eigen/Dense>

#include <QProgressDialog>
#include <atomic>
#include <future>

namespace Ui
{
class GetInformativePairsDialog;
}

class FRETEfficiencies;

class GetInformativePairsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GetInformativePairsDialog(
	        QWidget *parent, const std::vector<FrameDescriptor> &frames,
	        const Eigen::MatrixXf &effs,
	        const std::vector<std::string> &evalNames);
	~GetInformativePairsDialog();
	void setMaxPairs(int numPairsMax);
	void setError(float err);
	void setOutFile(const QString &path);
public Q_SLOTS:
	virtual void accept() override;
	void setFileName();

private:
	const std::vector<FrameDescriptor> frames;
	const Eigen::MatrixXf &effs;
	const std::vector<std::string> evalNames;
	Ui::GetInformativePairsDialog *ui;

	std::vector<unsigned> greedySelection(const float err,
					      const Eigen::MatrixXf &Effs,
					      const Eigen::MatrixXf &RMSDs,
					      const int maxPairs,
					      const int fitParams) const;
	pteros::System buildTrajectory(const std::string &sel) const;

	mutable std::atomic<float> fracDone{0.0f};
	template <typename Lambda>
	void showProgress(const QString &title, Lambda &&func);
};

#endif // GETINFORMATIVEPAIRSDIALOG_H
