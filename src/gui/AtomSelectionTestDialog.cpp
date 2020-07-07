#include "AtomSelectionTestDialog.h"
#include "ui_AtomSelectionTestDialog.h"

#include <QFileDialog>

AtomSelectionTestDialog::AtomSelectionTestDialog(const QString &topology,
						 QWidget *parent)
    : QDialog(parent), ui(new Ui::AtomSelectionTestDialog)
{
	ui->setupUi(this);
	ui->splitter->setSizes({0, 1});
	ui->topologyEdit->setText(topology);
	setTopology();
	updateTimer.setSingleShot(true);
	updateTimer.setInterval(300);
	connect(&updateTimer, &QTimer::timeout, this,
		&AtomSelectionTestDialog::fillAtoms);
	connect(ui->selectionEdit, &QPlainTextEdit::textChanged, [=]() {
		ui->statusLabel->setText("...");
		updateTimer.start();
	});
}

AtomSelectionTestDialog::~AtomSelectionTestDialog()
{
	delete ui;
}

void AtomSelectionTestDialog::browseTopology()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Pick a topology file"), "",
		tr("Molecular Topology files (*.pdb)"));
	ui->topologyEdit->setText(fileName);
}

void AtomSelectionTestDialog::fillAtoms()
{
	setTopology();
	ui->tableWidget->clearContents();
	const std::string &selExp =
		ui->selectionEdit->toPlainText().toStdString();
	pteros::Selection sel;
	try {
		sel = sys.select(selExp);
		ui->statusLabel->setText(QString("atoms: %1").arg(sel.size()));
	} catch (pteros::Pteros_error err) {
		ui->statusLabel->setText(QString("error: %1").arg(err.what()));
	}
	ui->tableWidget->setRowCount(sel.size());
	for (int i = 0; i < sel.size(); ++i) {
		QVariantList data;
		data << QChar(sel.chain(i));
		data << sel.resid(i);
		data << sel.resname(i).c_str();
		data << sel.name(i).c_str();
		for (int col = 0; col < data.size(); ++col) {
			auto item = new QTableWidgetItem;
			item->setData(Qt::DisplayRole, data[col]);
			ui->tableWidget->setItem(i, col, item);
		}
	}
}

void AtomSelectionTestDialog::setTopology()
{
	const QString &newTop = ui->topologyEdit->text();
	if (newTop != topology) {
		topology = newTop;
		sys.clear();
		if (!topology.isEmpty()) {
			sys.load(topology.toStdString());
		}
	}
}
