#ifndef GETINFORMATIVEPAIRSDIALOG_H
#define GETINFORMATIVEPAIRSDIALOG_H

#include <QDialog>

#include "FrameDescriptor.h"
#include <Eigen/Dense>

namespace Ui {
class GetInformativePairsDialog;
}

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
private:
	const std::vector<FrameDescriptor> frames;
	const Eigen::MatrixXf effs;
	const std::vector<std::string> evalNames;
	Ui::GetInformativePairsDialog *ui;
};

#endif // GETINFORMATIVEPAIRSDIALOG_H
