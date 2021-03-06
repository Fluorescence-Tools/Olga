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
class TaskStorage;

class GetInformativePairsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GetInformativePairsDialog(
		QWidget *parent, const std::vector<FrameDescriptor> &frames,
		const Eigen::MatrixXf &effs,
		const std::vector<std::string> &evalNames,
		const TaskStorage &storage);
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
	const TaskStorage &storage;
	const std::vector<std::string> evalNames;
	Ui::GetInformativePairsDialog *ui;

	pteros::System buildTrajectory(const std::string &sel) const;

	mutable std::atomic<float> fracDone{0.0f};
	template <typename T>
	void showProgress(const QString &title, const std::future<T> &future);
};

#endif // GETINFORMATIVEPAIRSDIALOG_H
