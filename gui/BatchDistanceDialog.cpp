#include "BatchDistanceDialog.h"
#include "ui_BatchDistanceDialog.h"
#include "EvaluatorPositionSimulation.h"
#include "EvaluatorDistance.h"

BatchDistanceDialog::BatchDistanceDialog(QWidget *parent, EvaluatorsTreeModel &evModel) :
	QDialog(parent),_evModel(evModel),
	ui(new Ui::BatchDistanceDialog)
{
	ui->setupUi(this);

	connect(ui->lpList1,&QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
		auto newState=item->checkState();
		for (auto selItem:ui->lpList1->selectedItems()) {
			selItem->setCheckState(newState);
		}
	});
	connect(ui->lpList2,&QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
		auto newState=item->checkState();
		for (auto selItem:ui->lpList2->selectedItems()) {
			selItem->setCheckState(newState);
		}
	});

	lpIndexes=_evModel.evaluatorsAvailable<EvaluatorPositionSimulation>();
	distanceIndexes=_evModel.evaluatorsAvailable<EvaluatorDistance>();

	for (const auto& index:distanceIndexes) {
		ui->referenceDistance->addItem(index.data().toString());
	}
	for (const auto& index:lpIndexes) {
		QListWidgetItem* item;
		item=new QListWidgetItem(index.data().toString());
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lpList1->addItem(item);
		ui->lpList2->addItem(item->clone());
	}
}

BatchDistanceDialog::~BatchDistanceDialog()
{
	delete ui;
}

void BatchDistanceDialog::accept()
{
	if(distanceIndexes.empty()) {
		QDialog::accept();
		return;
	}
	auto rootEval=distanceIndexes[ui->referenceDistance->currentIndex()];

	for(int i=0;  i<ui->lpList1->count(); ++i) {
		if(ui->lpList1->item(i)->checkState()==Qt::Unchecked) {
			continue;
		}
		QModelIndex lp1Index=lpIndexes[i];
		QString lp1name=QString::fromStdString(_evModel.evalName(lp1Index));
		EvalId lp1Id=_evModel.evalId(lp1Index);
		for(int j=i+1;  j<ui->lpList2->count(); ++j) {
			if(ui->lpList2->item(j)->checkState()==Qt::Unchecked) {
				continue;
			}
			QModelIndex lp2Index=lpIndexes[j];
			QString lp2name=QString::fromStdString(
						_evModel.evalName(lp2Index));
			EvalId lp2Id=_evModel.evalId(lp2Index);

			QModelIndex newDist=_evModel.duplicateEvaluator(rootEval);
			QString nameTemplate=ui->nameTemplateEdit->text();
			auto name=nameTemplate.replace("{lp1}",lp1name)
				  .replace("{lp2}",lp2name);
			_evModel.setEvaluatorOption(newDist,"position1_name",
						    QVariant::fromValue(lp1Id));
			_evModel.setEvaluatorOption(newDist,"position2_name",
						    QVariant::fromValue(lp2Id));
			_evModel.setEvaluatorName(newDist,name.toStdString());
			_evModel.activateEvaluator(newDist);
		}
	}
	QDialog::accept();
}
