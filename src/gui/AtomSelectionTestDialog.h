#ifndef ATOMSELECTIONTESTDIALOG_H
#define ATOMSELECTIONTESTDIALOG_H

#include <QDialog>
#include <QTimer>
#include <pteros/pteros.h>

namespace Ui
{
class AtomSelectionTestDialog;
}

class AtomSelectionTestDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AtomSelectionTestDialog(const QString &topology,
					 QWidget *parent = nullptr);
	~AtomSelectionTestDialog();
private Q_SLOTS:
	void browseTopology();
	void fillAtoms();

private:
	QTimer updateTimer{this};
	QString topology;
	pteros::System sys;
	void setTopology();

private:
	Ui::AtomSelectionTestDialog *ui;
};

#endif // ATOMSELECTIONTESTDIALOG_H
