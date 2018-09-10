#ifndef GETINFORMATIVEPAIRSDIALOG_H
#define GETINFORMATIVEPAIRSDIALOG_H

#include <QDialog>

#include "FrameDescriptor.h"
#include <pteros/pteros.h>
#include <Eigen/Dense>
#include <atomic>

namespace Ui {
class GetInformativePairsDialog;
}

class FRETEfficiencies;

class GetInformativePairsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GetInformativePairsDialog(
			QWidget *parent,
			const std::vector<FrameDescriptor>& frames,
			const Eigen::MatrixXf& effs,
			const std::vector<std::string>& evalNames);
	~GetInformativePairsDialog();
public Q_SLOTS:
	virtual void accept() override;
	void setFileName();
private:
	const std::vector<FrameDescriptor> frames;
	const Eigen::MatrixXf& effs;
	const std::vector<std::string> evalNames;
	Ui::GetInformativePairsDialog *ui;

	using pair_list_type=std::vector<std::pair<std::string,float>>;
	static std::string list2str(const GetInformativePairsDialog::pair_list_type &list);

	pair_list_type miSelection(const float err, const FRETEfficiencies &Eall,
				   const Eigen::MatrixXf &RMSDs,
				   const int maxPairs) const;

	pair_list_type greedySelection(const float err, const FRETEfficiencies &Eall,
				       const Eigen::MatrixXf &RMSDs,
				       const int maxPairs, const int fitParams) const;
	mutable std::atomic<int> percDone{0};

	Eigen::MatrixXf rmsds(const pteros::System& traj) const;
	mutable std::atomic<int> rmsdsDone{0};

};

#endif // GETINFORMATIVEPAIRSDIALOG_H
