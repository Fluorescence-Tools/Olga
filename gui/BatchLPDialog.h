#ifndef BATCHLPDIALOG_H
#define BATCHLPDIALOG_H

#include <QDialog>
#include "EvaluatorsTreeModel.h"

namespace Ui {
class BatchLPDialog;
}

class BatchLPDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchLPDialog(QWidget *parent, EvaluatorsTreeModel& evModel);
	~BatchLPDialog();
	void setResidueList(const std::vector<std::pair<int,std::string>>& residues);
public Q_SLOTS:
	virtual void reject() override
	{
		QDialog::reject();
	}
	virtual void accept() override;
private:
	std::vector<std::pair<int, std::string> > _residues;
	EvaluatorsTreeModel& _evModel;
	Ui::BatchLPDialog *ui;
	QList<QModelIndex> indexes;
};

#endif // BATCHLPDIALOG_H
