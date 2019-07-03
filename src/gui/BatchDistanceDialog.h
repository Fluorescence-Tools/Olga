#ifndef BATCHDISTANCEDIALOG_H
#define BATCHDISTANCEDIALOG_H

#include <QDialog>
#include "EvaluatorsTreeModel.h"

namespace Ui
{
class BatchDistanceDialog;
}

class BatchDistanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchDistanceDialog(QWidget *parent,
				     EvaluatorsTreeModel &evModel);
	~BatchDistanceDialog();
public Q_SLOTS:
	virtual void reject() override
	{
		QDialog::reject();
	}
	virtual void accept() override;

private:
	EvaluatorsTreeModel &_evModel;
	QList<QModelIndex> lpIndexes;
	QList<QModelIndex> distanceIndexes;

	Ui::BatchDistanceDialog *ui;
};

#endif // BATCHDISTANCEDIALOG_H
