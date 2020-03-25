#ifndef LOADTRAJECTORYDIALOG_H
#define LOADTRAJECTORYDIALOG_H

#include <QDialog>

namespace Ui
{
class LoadTrajectoryDialog;
}

class LoadTrajectoryDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LoadTrajectoryDialog(QWidget *parent = nullptr);
	~LoadTrajectoryDialog();
	QString topologyPath() const;
	QString trajectoryPath() const;
private Q_SLOTS:
	void browseTopology();
	void browseTrajectory();

private:
	Ui::LoadTrajectoryDialog *ui;
};

#endif // LOADTRAJECTORYDIALOG_H
