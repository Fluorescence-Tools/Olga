#include "BatchFretEfficiencyDialog.h"
#include "ui_BatchFretEfficiencyDialog.h"

#include "EvaluatorPositionSimulation.h"
#include "EvaluatorFretEfficiency.h"

#include <QProgressDialog>

BatchFretEfficiencyDialog::BatchFretEfficiencyDialog(
	QWidget *parent, EvaluatorsTreeModel &evModel)
    : QDialog(parent), _evModel(evModel), ui(new Ui::BatchFretEfficiencyDialog)
{
	ui->setupUi(this);

	connect(ui->lpList1, &QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
			if (!item->isSelected()) {
				return;
			}
			auto newState = item->checkState();
			for (auto selItem : ui->lpList1->selectedItems()) {
				selItem->setCheckState(newState);
			}
		});
	connect(ui->lpList2, &QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
			if (!item->isSelected()) {
				return;
			}
			auto newState = item->checkState();
			for (auto selItem : ui->lpList2->selectedItems()) {
				selItem->setCheckState(newState);
			}
		});

	lpIndexes = _evModel.evaluatorsAvailable<EvaluatorPositionSimulation>();

	for (const auto &index : lpIndexes) {
		QListWidgetItem *item;
		item = new QListWidgetItem(index.data().toString());
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lpList1->addItem(item);
		ui->lpList2->addItem(item->clone());
	}
}

BatchFretEfficiencyDialog::~BatchFretEfficiencyDialog()
{
	delete ui;
}

void BatchFretEfficiencyDialog::accept()
{
	const long numEffs = ui->lpList1->count() * ui->lpList2->count() / 2;
	QProgressDialog addProgress("Creating efficiencies...", QString(), 0,
				    numEffs, this);
	addProgress.setWindowModality(Qt::WindowModal);
	addProgress.setMinimumDuration(10);
	addProgress.setValue(0);

	for (int i = 0; i < ui->lpList1->count(); ++i) {
		if (ui->lpList1->item(i)->checkState() == Qt::Unchecked) {
			continue;
		}
		QModelIndex lp1Index = lpIndexes[i];
		QString lp1name =
			QString::fromStdString(_evModel.evalName(lp1Index));
		EvalId lp1Id = _evModel.evalId(lp1Index);
		for (int j = i + 1; j < ui->lpList2->count(); ++j) {
			if (ui->lpList2->item(j)->checkState()
			    == Qt::Unchecked) {
				continue;
			}
			QModelIndex lp2Index = lpIndexes[j];
			QString lp2name = QString::fromStdString(
				_evModel.evalName(lp2Index));
			EvalId lp2Id = _evModel.evalId(lp2Index);

			QModelIndex newEff = _evModel.addEvaluator<
				EvaluatorFretEfficiency>();
			QString nameTemplate = ui->nameTemplateEdit->text();
			auto name = nameTemplate.replace("{lp1}", lp1name)
					    .replace("{lp2}", lp2name);
			_evModel.setEvaluatorOption(newEff, "position1_name",
						    QVariant::fromValue(lp1Id));
			_evModel.setEvaluatorOption(newEff, "position2_name",
						    QVariant::fromValue(lp2Id));
			_evModel.setEvaluatorOption(newEff, "Forster_radius",
						    ui->r0SpinBox->value());
			_evModel.setEvaluatorName(newEff, name.toStdString());
			_evModel.activateEvaluator(newEff);

			addProgress.setValue(addProgress.value() + 1);
		}
	}
	addProgress.setValue(numEffs);
	QDialog::accept();
}

void BatchFretEfficiencyDialog::autoAccept()
{
	toggleAllFrom(Qt::Checked);
	toggleAllTo(Qt::Checked);
	accept();
}

void BatchFretEfficiencyDialog::toggleAllFrom(int state)
{
	auto checkState = static_cast<Qt::CheckState>(state);
	for (int i = 0; i < ui->lpList1->count(); ++i) {
		ui->lpList1->item(i)->setCheckState(checkState);
	}
}

void BatchFretEfficiencyDialog::toggleAllTo(int state)
{
	auto checkState = static_cast<Qt::CheckState>(state);
	for (int j = 0; j < ui->lpList2->count(); ++j) {
		ui->lpList2->item(j)->setCheckState(checkState);
	}
}
