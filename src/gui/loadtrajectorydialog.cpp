#include "loadtrajectorydialog.h"
#include "ui_loadtrajectorydialog.h"
#include <QFileDialog>

LoadTrajectoryDialog::LoadTrajectoryDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoadTrajectoryDialog)
{
	ui->setupUi(this);
}

LoadTrajectoryDialog::~LoadTrajectoryDialog()
{
	delete ui;
}

QString LoadTrajectoryDialog::topologyPath() const
{
	return ui->topologyLineEdit->text();
}

QString LoadTrajectoryDialog::trajectoryPath() const
{
	return ui->trajectoryLineEdit->text();
}

void LoadTrajectoryDialog::browseTopology()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Pick a topology file"), "",
		tr("Molecular Topology files (*.pdb)"));
	ui->topologyLineEdit->setText(fileName);
}

void LoadTrajectoryDialog::browseTrajectory()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Pick a trajectory file"), "",
		tr("Molecular Trajectory files (*.dcd)"));
	ui->trajectoryLineEdit->setText(fileName);
}
