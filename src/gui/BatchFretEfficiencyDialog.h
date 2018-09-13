#ifndef BATCHFRETEFFICIENCYDIALOG_H
#define BATCHFRETEFFICIENCYDIALOG_H

#include <QDialog>
#include "EvaluatorsTreeModel.h"

namespace Ui
{
class BatchFretEfficiencyDialog;
}

class BatchFretEfficiencyDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchFretEfficiencyDialog(QWidget *parent,
					   EvaluatorsTreeModel &evModel);
	~BatchFretEfficiencyDialog();
public Q_SLOTS:
	virtual void reject() override
	{
		QDialog::reject();
	}
	virtual void accept() override;
	void autoAccept();
	void toggleAllFrom(int state);
	void toggleAllTo(int state);

private:
	EvaluatorsTreeModel &_evModel;
	QList<QModelIndex> lpIndexes;

	Ui::BatchFretEfficiencyDialog *ui;
};

#endif // BATCHFRETEFFICIENCYDIALOG_H
