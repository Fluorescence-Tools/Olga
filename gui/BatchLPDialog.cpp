#include "BatchLPDialog.h"
#include "ui_BatchLPDialog.h"
#include "EvaluatorPositionSimulation.h"

BatchLPDialog::BatchLPDialog(QWidget *parent, EvaluatorsTreeModel& evModel) :
	QDialog(parent),_evModel(evModel),ui(new Ui::BatchLPDialog)
{
	ui->setupUi(this);

	connect(ui->residuesWidget,&QListWidget::itemChanged,
		[this](QListWidgetItem *item) {
		if (!item->isSelected()) {
			return;
		}
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

void BatchLPDialog::setResidueList(const std::vector<std::tuple<int, std::string, char>> &residues)
{
	for (const auto& tup:residues) {
		QString str = QString::number(std::get<0>(tup)) + " (chain "
			    + QString(std::get<2>(tup)) + ", "
			    + QString::fromStdString(std::get<1>(tup)) + ")";
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
		int resi=std::get<0>(_residues[i]);
		auto resname=QString::fromStdString(std::get<1>(_residues[i]));
		QString chain(std::get<2>(_residues[i]));
		chain=(chain==" ")?"":chain;
		auto index=_evModel.duplicateEvaluator(rootEval);
		QString nameTemplate=ui->nameTemplateEdit->text();
		auto name=nameTemplate.replace("{resid}",QString::number(resi));
		_evModel.setEvaluatorName(index,name.toStdString());
		_evModel.setEvaluatorOption(index,"residue_seq_number",resi);
		_evModel.setEvaluatorOption(index,"residue_name",resname);
		_evModel.setEvaluatorOption(index,"chain_identifier",chain);
		_evModel.activateEvaluator(index);
	}
	QDialog::accept();
}
