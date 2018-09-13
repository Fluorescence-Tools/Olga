#ifndef BATCHLPDIALOG_H
#define BATCHLPDIALOG_H

#include <QDialog>
#include "EvaluatorsTreeModel.h"

namespace Ui
{
class BatchLPDialog;
}

class BatchLPDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchLPDialog(QWidget *parent, EvaluatorsTreeModel &evModel);
	~BatchLPDialog();
	void
	setResidueList(const std::vector<std::tuple<int, std::string, char>>
			       &residues);
public Q_SLOTS:
	virtual void reject() override
	{
		QDialog::reject();
	}
	void autoAccept();
	virtual void accept() override;
	void toggleAll(int state);

private:
	std::vector<std::tuple<int, std::string, char>>
		_residues; // resid, resname, chain
	EvaluatorsTreeModel &_evModel;
	Ui::BatchLPDialog *ui;
	QList<QModelIndex> indexes;
};

#endif // BATCHLPDIALOG_H
