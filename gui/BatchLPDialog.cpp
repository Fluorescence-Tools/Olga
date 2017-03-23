#include "BatchLPDialog.h"
#include "ui_BatchLPDialog.h"
#include "EvaluatorPositionSimulation.h"

BatchLPDialog::BatchLPDialog(QWidget *parent, EvaluatorsTreeModel& evModel) :
	QDialog(parent),_evModel(evModel),ui(new Ui::BatchLPDialog)
{
	ui->setupUi(this);

	connect(ui->residuesWidget,&QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
		auto newState=item->checkState();
		for (auto selItem:ui->residuesWidget->selectedItems()) {
			selItem->setCheckState(newState);
		}
	});

	indexes=_evModel.evaluatorsAvailable<EvaluatorPositionSimulation>();
	for (const auto& index:indexes) {
		ui->settingSource->addItem(index.data().toString());
	}


}

BatchLPDialog::~BatchLPDialog()
{
	delete ui;
}

void BatchLPDialog::setResidueList(const std::vector<std::pair<int, std::string> > &residues)
{
	for (const auto& pair:residues) {
		QString str = QString::number(pair.first) + " ("
			    + QString::fromStdString(pair.second) + ")";
		QListWidgetItem* item=new QListWidgetItem(std::move(str));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->residuesWidget->addItem(item);
	}
	_residues=residues;
}

void BatchLPDialog::accept()
{
	if(indexes.empty()) {
		QDialog::accept();
		return;
	}
	auto rootEval=indexes[ui->settingSource->currentIndex()];
	for(int i=0;  i<ui->residuesWidget->count(); ++i) {
		if(ui->residuesWidget->item(i)->checkState()==Qt::Unchecked) {
			continue;
		}
		int resi=_residues[i].first;
		auto resname=QString::fromStdString(_residues[i].second);
		auto index=_evModel.duplicateEvaluator(rootEval);
		QString nameTemplate=ui->nameTemplateEdit->text();
		auto name=nameTemplate.replace("{resid}",QString::number(resi));
		_evModel.setEvaluatorName(index,name.toStdString());
		_evModel.setEvaluatorOption(index,"residue_seq_number",resi);
		_evModel.setEvaluatorOption(index,"residue_name",resname);
		_evModel.activateEvaluator(index);
	}
	QDialog::accept();
}
