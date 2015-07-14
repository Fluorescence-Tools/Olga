#include <vector>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QClipboard>
#include <QSignalMapper>
#include <QMimeData>
#include <QTimer>
#include <QTime>
#include "Q_DebugStream.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	qRegisterMetaType<std::shared_ptr<AbstractCalcResult>>("shared_ptr<AbstractCalcResult>");
	qRegisterMetaType<std::shared_ptr<AbstractEvaluator>>("shared_ptr<AbstractEvaluator>");
	qRegisterMetaType<FrameDescriptor>("FrameDescriptor");
	qRegisterMetaType<EvalPtr>("EvalPtr");
	qRegisterMetaType<Position::SimulationType>("SimulationType");

	ui->setupUi(this);

	ui->evalTypeAddComboBox->addItems(evalsModel.supportedTypes());

	evaluatorsDelegate = new EvaluatorDelegate(ui->evaluatorsTreeView);
	//tabifyDockWidget();

	readSettings();
	ui->evalPropDockWidget->hide();
	ui->evalPropDockWidget->close();

	ui->mainTreeView->setModel(&trajectoriesModel);

	ui->evaluatorsTreeView->setModel(&evalsModel);
	ui->evaluatorsTreeView->setItemDelegate(evaluatorsDelegate);
	ui->evaluatorsTreeView->resizeColumnToContents(1);

	setupMenus();

	ui->statusBar->addPermanentWidget(&tasksStatus);
	const int timerInterval = 5000;
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [ = ]() {
		static int tasksPendindOld = 0;
		int tasksPending = trajectoriesModel.tasksPendingCount();
		int ETA = tasksPending < tasksPendindOld ?
				  timerInterval / 1000 * tasksPending / (tasksPendindOld - tasksPending) : 0;
		tasksPendindOld = tasksPending;

		QString message = QString("Tasks pending/ready/running: %1/%2/%3; ETA: ")
				  .arg(tasksPending)
				  .arg(trajectoriesModel.resultsCount())
				  .arg(trajectoriesModel.tasksRunningCount())
				  + timespan(ETA);
		tasksStatus.setText(message);
		ui->mainTreeView->viewport()->update();
	});
	timer->start(timerInterval);

	auto degugStream = new QDebugStream(std::cerr); //Capture stderr
	connect(degugStream, SIGNAL(errorPrinted(QString)),
		ui->logTextEdit, SLOT(append(const QString &)));

	connect(&trajectoriesModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
		this, SLOT(expand(const QModelIndex &, int, int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::readSettings()
{
	QSettings settings("MPC", "Olga");
	QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
	QSize size = settings.value("size", QSize(400, 400)).toSize();

	resize(size);
	move(pos);
	restoreState(settings.value("windowState").toByteArray());
}
void MainWindow::writeSettings() const
{
	QSettings settings("MPC", "Olga");

	settings.setValue("pos", pos());
	settings.setValue("size", size());
	settings.setValue("windowState", saveState());
}

void MainWindow::setupMenus()
{


	ui->mainTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	//ui->distancesTableView->setContextMenuPolicy(Qt::CustomContextMenu);

	/*connect(ui->mainTreeView, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(ShowSystemsContextMenu(const QPoint&)));*/

	/*connect(ui->distancesTableView, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(ShowDistancesContextMenu(const QPoint&)));*/

	//distancesMenu.addAction("Delete selected distances",this,SLOT(deleteSelectedDistances()));


	QAction *action = systemsMenu.addAction("&Copy");
	connect(action, &QAction::triggered, [ = ]() {
		copySelectedText(ui->mainTreeView->selectionModel());
	});

	/*action=distancesMenu.addAction("&Copy");
	   connect(action, &QAction::triggered, [=]() {
		copySelectedText(ui->distancesTableView->selectionModel());
	   }  );*/

	//ui->distancesTableView->installEventFilter(this);

	ui->mainTreeView->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if ((object == ui->mainTreeView /*|| object == ui->distancesTableView */) &&
	    event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->matches(QKeySequence::Copy)) {
			QAbstractItemView *viewObject = static_cast<QAbstractItemView *>(object);
			copySelectedText(viewObject->selectionModel());
			return true;

		} else if (keyEvent->matches(QKeySequence::Paste)) {
			QAbstractItemView *viewObject = static_cast<QAbstractItemView *>(object);
			pasteText(viewObject);
			return true;
		}
	}

	return false;
}

QString MainWindow::tabSeparatedData(const QItemSelectionModel *selectionModel) const
{
	const QAbstractItemModel *model = selectionModel->model();
	QModelIndexList indexes = selectionModel->selectedIndexes();

	if (indexes.size() < 1) {
		return "";
	}

	std::sort(indexes.begin(), indexes.end(),
		  [] (const QModelIndex &lhs, const QModelIndex &rhs) {
		if (lhs.sibling(rhs.row(), rhs.column()) == rhs) {
			return lhs < rhs;
		}

		QModelIndexList lParents, rParents;
		lParents.prepend(lhs);

		for (QModelIndex idx = lhs; idx.isValid(); idx = idx.parent()) {
			lParents.prepend(idx);
		}

		rParents.prepend(rhs);

		for (QModelIndex idx = rhs; idx.isValid(); idx = idx.parent()) {
			rParents.prepend(idx);
		}

		int nParents = std::min(lParents.size(), rParents.size()) - 1;

		for (int i = 0; i < nParents; i++) {
			if (lParents[i].row() != rParents[i].row()) {
				return lParents[i].row() < rParents[i].row();
			}
		}

		return lParents.size() < rParents.size();
	});

	QString selectedText;
	QModelIndex previous = indexes.takeFirst();

	for (const QModelIndex & index : indexes) {

		QString text = model->data(previous).toString();
		// At this point `text` contains the text in one cell
		selectedText.append(text);

		// If you are at the start of the row the row number of the previous index
		// isn't the same.  Text is followed by a row separator, which is a newline.
		if (index.row() != previous.row() ||
		    index.parent() != previous.parent()) {
			selectedText.append(QLatin1Char('\n'));
		}
		// Otherwise it's the same row, so append a column separator, which is a tab.
		else {
			selectedText.append(QLatin1Char('\t'));
		}

		previous = index;
	}

	selectedText.append(model->data(previous).toString());
	selectedText.append(QLatin1Char('\n'));
	return selectedText;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	/*if (maybeSave()) {
	   writeSettings();
	   event->accept();
	   } else {
	   event->ignore();
	   }*/
	writeSettings();
	event->accept();
}

void MainWindow::loadStructures()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Load strcutures from files or folders"), "",
							      tr("Molecular Structure Files (*.pdb *.mol2 *.mol *.hin *.xyz *.kcf *.sd *.ac)"));

	for (const QString & fileName : fileNames) {
		if (!fileName.isEmpty()) {
			statusBar()->showMessage(tr("Loading %1...").arg(fileName), 2000);
			trajectoriesModel.loadSystem(fileName);
		}
	}

	trajectoriesModel.updateColumn(0);
	//ui->mainTreeView->resizeColumnsToContents();
}

void MainWindow::metropolisSampling()
{

}

void MainWindow::loadJson()
{
	QString fileName = QFileDialog::getOpenFileName(this,
							tr("Open Settings File"), "",
							tr("FPS settings (*.fps.json);;All Files (*)"));

	if (fileName.isEmpty()) {
		return;
	}

	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::information(this, tr("Unable to open file"),
					 file.errorString());
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());

	if (doc.isNull()) {
		QMessageBox::information(this, tr("Unable to parse JSON file"),
					 tr("Could not open the file. "
					    "There is a syntax error in the provided JSON file."));
		return;
	}
	evalsModel.loadEvaluators(doc.toVariant().toMap());

	/*QJsonObject positionsListObj=docObj.value("Positions").toObject();
	   positionsModel.load(positionsListObj);
	   ui->labellingPositionsTableView->resizeColumnsToContents();

	   QJsonObject distancesListObj=docObj.value("Distances").toObject();
	   distancesModel.load(distancesListObj);
	   ui->distancesTableView->resizeColumnsToContents();

	   QJsonArray domainsArr=docObj.value("Domains").toArray();
	   domainsModel.load(domainsArr);
	   ui->domainsTableView->resizeColumnsToContents();*/

	//ui->mainTreeView->resizeColumnsToContents();
	trajectoriesModel.updateColumn(0);
}

bool MainWindow::saveJson()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Settings"), "untitled.fps.json",
							tr("FPS settings (*.fps.json);;Any file (*)"));

	if (fileName.isEmpty()) {
		return false;
	}

	/*QFileInfo fileInfo( fileName );
	if (fileInfo.suffix().isEmpty())
	{
		fileName += ".fps.json";
	}*/

	QFile file(fileName);

	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("Application"),
				     tr("Cannot write file %1:\n%2.")
				     .arg(fileName)
				     .arg(file.errorString()));
		return false;
	}

	QJsonObject obj;

	file.write(QJsonDocument::fromVariant(evalsModel.evaluators()).toJson());
	file.close();
	statusBar()->showMessage(tr("File %1 saved").arg(fileName), 5000);
	return true;
}

bool MainWindow::exportData()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export data"), "",
							tr("Tab-separated values (*.ha4);;Any file (*)"));

	if (fileName.isEmpty()) {
		return false;
	}

	/*QFileInfo fileInfo( fileName );
	   if (fileInfo.suffix().isEmpty())
	   {
	   fileName += ".csv";
	   }*/

	QFile file(fileName);

	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("Application"),
				     tr("Cannot write file %1:\n%2.")
				     .arg(fileName)
				     .arg(file.errorString()));
		return false;
	}

	file.write(trajectoriesModel.tabSeparatedData());
	statusBar()->showMessage(tr("File %1 saved").arg(fileName), 5000);
	return true;
}

bool MainWindow::exportCylinders()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export cylinders for pymol"),
							"", tr("Python script (*.py);;Any file (*)"));

	if (fileName.isEmpty()) {
		return false;
	}

	/*QFileInfo fileInfo( fileName );
	   if (fileInfo.suffix().isEmpty())
	   {
	   fileName += ".csv";
	   }*/

	QFile file(fileName);

	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("Application"),
				     tr("Cannot write file %1:\n%2.")
				     .arg(fileName)
				     .arg(file.errorString()));
		return false;
	}

	file.write("from pymol.cgo import *\nfrom pymol import cmd\nobj = [\n");

	for (const QString & str : trajectoriesModel.cylinders()) {
		file.write((str + '\n').toUtf8());
	}

	file.write("]\ncmd.load_cgo(obj,'cylinders')");
	statusBar()->showMessage(tr("File %1 saved").arg(fileName), 5000);
	return true;
}

bool MainWindow::exportStructures()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
							    "",
							    QFileDialog::ShowDirsOnly
							    | QFileDialog::DontResolveSymlinks);

	if (dirName.isEmpty()) {
		return false;
	}



	for (int r = 0; r < trajectoriesModel.rowCount(); r++) {
		QString filename = dirName + "/" + trajectoriesModel.data(trajectoriesModel.index(r,
												  0)).toString() + ".pdb";
		trajectoriesModel.exportSystem(r, filename);
		statusBar()->showMessage(tr("Exporing %1").arg(filename), 2000);
	}

	return true;
}

void MainWindow::addEvaluator()
{
	evalsModel.addEvaluator(ui->evalTypeAddComboBox->currentIndex());
}

/*void MainWindow::deleteSelectedPositions()
   {
	QModelIndexList list=ui->labellingPositionsTableView->selectionModel()->selectedRows();
	if(list.empty()){
		return;
	}
	std::sort(list.begin(), list.end());
	int prevRow=list.first().row(), startRow=prevRow;
	int removed=0;
	for(auto item=list.begin()+1; item!=list.end(); item++)
	{
		if(prevRow==item->row()-1)
		{
			prevRow=item->row();
		}
		else
		{
			positionsModel.removeRows(startRow-removed,prevRow-startRow+1);
			removed+=prevRow-startRow+1;
			prevRow=startRow=item->row();
		}
	}
	positionsModel.removeRows(startRow-removed,prevRow-startRow+1);
   }

   void MainWindow::ShowSystemsContextMenu(const QPoint &pos)
   {
	QPoint globalPos = ui->mainTreeView->mapToGlobal(pos);
	systemsMenu.exec(globalPos);
   }


   void MainWindow::ShowPositionsContextMenu(const QPoint &pos)
   {
	QPoint globalPos = ui->labellingPositionsTableView->mapToGlobal(pos);
	positionsMenu.exec(globalPos);
   }
 */
void MainWindow::copySelectedText(const QItemSelectionModel *selModel) const
{
	QString selected_text = tabSeparatedData(selModel);

	QApplication::clipboard()->setText(selected_text);
}

void MainWindow::pasteText(QAbstractItemView *view) const
{
	QString text = qApp->clipboard()->text();
	QStringList cells = text.split(QRegExp(QLatin1String("\\n|\\t")));

	while (!cells.empty() && cells.back().size() == 0)
		cells.pop_back(); // strip empty trailing tokens

	int rows = text.count(QLatin1Char('\n'));
	int cols = cells.size() / rows;

	if (cells.size() % rows != 0) {
		// error, uneven number of columns, probably bad data
		QMessageBox::critical(0, tr("Error"),
				      tr("Invalid clipboard data: uneven number of columns. Unable to perform paste operation."));
		return;
	}

	QAbstractItemModel *model = view->model();
	const QItemSelectionModel *selectionModel = view->selectionModel();
	QModelIndexList indexes = selectionModel->selectedIndexes();
	std::sort(indexes.begin(), indexes.end());
	int rowStart = indexes.first().row();
	int colStart = indexes.first().column();

	int cell = 0;

	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col, ++cell) {
			model->setData(model->index(row + rowStart, col + colStart), cells[cell]);
		}
	}

}

void MainWindow::expand(const QModelIndex &parentIndex, int first, int last)
{
	for (int row = first; row <= last; row++) {
		QModelIndex child = parentIndex.child(first, 0);

		if (!parentIndex.isValid()) {
			child = trajectoriesModel.index(row, 0);
		}

		QModelIndexList indexes = trajectoriesModel.match(child, Qt::DisplayRole, "*", 2,
								  Qt::MatchWildcard | Qt::MatchRecursive);

		for (const auto & idx : indexes) {
			ui->mainTreeView->expand(idx);
		}
	}
}
void MainWindow::showAbout()
{
	QMessageBox::about(this, "SRHDDumpReader",
			   QString("Olga v. %1\nMykola Dimura, dimura@hhu.de\n")
			   .arg(QApplication::applicationVersion()));
}

QString MainWindow::timespan(unsigned seconds)
{
	if (seconds < 61) {
		return QString::number(seconds) + "s";

	} else if (seconds < 3600) {
		return QString::number(seconds / 60) + "m";

	} else if (seconds < 86400) {
		return QString::number(seconds / 3600) + "h";
	}

	return QString::number(seconds / 86400) + "d "
			+ QString::number(seconds / 3600 % 24) + "h";
}
