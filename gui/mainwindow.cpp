#include <vector>

#include <Eigen/Dense>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "BatchLPDialog.h"
#include "BatchDistanceDialog.h"
#include "BatchFretEfficiencyDialog.h"
#include "GetInformativePairsDialog.h"
#include "EvaluatorFretEfficiency.h"
#include "CalcResult.h"

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
#include <QProgressDialog>
#include <QTextStream>
#include <QScrollBar>
#include "Q_DebugStream.h"



MainWindow::MainWindow(const QString json, const QString pdbsDir, const QString ha4Out, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	qRegisterMetaType<std::shared_ptr<AbstractCalcResult>>("shared_ptr<AbstractCalcResult>");
	qRegisterMetaType<std::shared_ptr<AbstractEvaluator>>("shared_ptr<AbstractEvaluator>");
	qRegisterMetaType<FrameDescriptor>("FrameDescriptor");
	qRegisterMetaType<EvalId>("EvalId");
	qRegisterMetaType<Position::SimulationType>("SimulationType");
	qRegisterMetaType<Eigen::Vector3d>("Eigen::Vector3d");

	ui->setupUi(this);

	ui->menuExtras->menuAction()->setVisible(false);
	if(QFileInfo("enableExtras").exists()) {
		ui->menuExtras->menuAction()->setVisible(true);
		ui->menuExtras->setEnabled(true);
	}

	ui->evalTypeAddComboBox->addItems(_storage.supportedTypes());

	evaluatorsDelegate = new EvaluatorDelegate(ui->evaluatorsTreeView);
	//tabifyDockWidget();

	readSettings();
	ui->evalPropDockWidget->hide();
	ui->evalPropDockWidget->close();

	ui->mainTreeView->setModel(&trajectoriesModel);
	ui->mainTreeView->setUniformRowHeights(true);

	ui->evaluatorsTreeView->setModel(&evalsModel);
	ui->evaluatorsTreeView->expandAll();
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
	auto infoStream = new QDebugStream(std::cout); //Capture stderr
	connect(degugStream, SIGNAL(errorPrinted(QString)),
		ui->logTextEdit, SLOT(append(const QString &)));
	connect(infoStream, SIGNAL(errorPrinted(QString)),
		ui->infoTextEdit, SLOT(append(const QString &)));

	connect(&trajectoriesModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
		this, SLOT(expand(const QModelIndex &, int, int)));
	if(json.size()>0 && pdbsDir.size()>0 && ha4Out.size() >0) {
		loadStructuresFolder(pdbsDir);
		loadEvaluators(json);
		QTimer* timer=new QTimer(this);
		timer->setSingleShot(false);
		connect(timer, &QTimer::timeout, [=](){
			if(!_storage.ready()) {
				return;
			}
			timer->stop();
			exportData(ha4Out);
			close();
		});
		timer->start(1000);
	}
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

void MainWindow::loadMolecules(const QStringList &fileNames)
{
	using namespace std;
	auto pause=_storage.pause();
	trajectoriesModel.loadSystems(fileNames);

	/*//This is a hack. It prevents from lags when scrolling
	const int vPosStart=ui->mainTreeView->verticalScrollBar()->value();
	const int maxScroll=ui->mainTreeView->verticalScrollBar()->maximum();
	QProgressDialog progress("Updating the view...","",0,maxScroll);
	for (int i=0; i<maxScroll; ++i) {
		ui->mainTreeView->verticalScrollBar()->setValue(i);
		progress.setValue(i);
	}
	ui->mainTreeView->verticalScrollBar()->setValue(vPosStart);
	progress.setValue(maxScroll);*/

	statusBar()->showMessage(tr("Loaded %1 frames").arg(fileNames.size()), 5000);
}

void MainWindow::loadStructuresFolder(const QString &path)
{
	QDir dir(path);
	QStringList fileNames=dir.entryList({"*.pdb"});
	const int size=fileNames.size();
	QProgressDialog progress("Listing files...",QString(),0,size,this);
	progress.setWindowModality(Qt::WindowModal);
	for(int i=0; i<size; ++i) {
		auto& s=fileNames[i];
		s.prepend(dir.absolutePath()+"/");
		progress.setValue(i);
	}
	progress.setValue(size);
	loadMolecules(fileNames);
}

void MainWindow::loadEvaluators(const QString &fileName)
{
	if (fileName.isEmpty()) {
		return;
	}

	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::information(this, tr("Unable to open Evaluators file"),
					 file.errorString()+":\n"+fileName);
		return;
	}

	QVariantMap evalsData;
	//Load old format
	if(fileName.endsWith(".txt")) {
		QTextStream in(&file);
		evalsData=evalsModel.evaluatorsFromLegacy(in);
	}
	//Load new format
	else {
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());

		if (doc.isNull()) {
			QMessageBox::information(this, tr("Unable to parse JSON file"),
						 tr("Could not open the file. "
						    "There is a syntax error in the provided JSON file."));
			return;
		}
		evalsData=doc.toVariant().toMap();
	}

	auto pause=_storage.pause();
	QApplication::setOverrideCursor(Qt::WaitCursor);
	evalsModel.loadEvaluators(evalsData);
	QApplication::restoreOverrideCursor();

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
}

bool MainWindow::exportData(const QString &fileName)
{
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
	QTextStream out(&file);
	trajectoriesModel.dumpTabSeparatedData(out);
	statusBar()->showMessage(tr("File %1 saved").arg(fileName), 5000);
	return true;
}

void MainWindow::setPaused(bool state)
{

	if(state) {
		_pause=_storage.pausePtr();
	} else if(_pause){
		_pause.reset();
	}
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
	QStringList fileNames =
			QFileDialog::getOpenFileNames(this,
						      tr("Load strcutures from files"), "",
						      tr("Molecular Structure Files (*.pdb *.nc)"));
	loadMolecules(fileNames);
	//ui->mainTreeView->resizeColumnsToContents();
}

void MainWindow::loadStructuresFolder()
{
	QString path = QFileDialog::getExistingDirectory(this,
							 tr("Load strcutures from a folder"), "");
	if(!path.isEmpty()) {
		loadStructuresFolder(path);
	}
}

void MainWindow::metropolisSampling()
{

}

void MainWindow::loadEvaluators()
{
	QString fileName = QFileDialog::getOpenFileName(this,
							tr("Open Settings File"), "",
							tr("FPS settings (*.fps.json);;FPS obsolete format settings (*.txt);;All Files (*)"));
	loadEvaluators(fileName);

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

	return exportData(fileName);
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
	if (rows==0) {
		return;
	}
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
	QMessageBox about(this);
	about.setWindowTitle("Olga");
	about.setTextFormat(Qt::RichText);
	about.setStandardButtons(QMessageBox::Ok);
	QString msg="Olga v. %1<br>"
		    "Mykola Dimura, dimura@hhu.de<br>"
		    "If you use this software for scientific purposes,<br>"
		    "please cite:<br>"
		    "Dimura, M., Peulen, T.O., Hanke, C.A., Prakash, A.,<br>"
		    "Gohlke, H. and Seidel, C.A., 2016.<br>"
		    "Current Opinion in Structural Biology, 40, pp.163-185.<br>"
		    "<a href='http://doi.org/10.1016/j.sbi.2016.11.012'>"
		    "doi:10.1016/j.sbi.2016.11.012</a>";
	msg=msg.arg(QApplication::applicationVersion());
	about.setText(msg);
	about.exec();
//	QMessageBox::about(this, "Olga",msg);
}

void MainWindow::addLpBatch()
{

	BatchLPDialog dialog(this,evalsModel);
	auto system=trajectoriesModel.firstSystem();
	std::vector<std::tuple<int,std::string,char>> residues;
	if (system.num_atoms()>0) {
		std::vector<pteros::Selection> resSel;
		system.select_all().split_by_residue(resSel);
		for(pteros::Selection& sel:resSel) {
			char chain=sel.begin()->Chain();
			int resid=sel.begin()->Resid();
			auto resname=sel.begin()->Resname();
			residues.emplace_back(resid,resname,chain);
		}
	}
	if(residues.empty()) {
		QMessageBox::warning(this,tr("Can not add labeling positions"),
				     tr("Could not populate the residue list.\n"
					"Did you import any molecule or trajectory?"));
		return;
	}
	if(evalsModel.evaluatorsAvailable<EvaluatorPositionSimulation>().empty()) {
		QMessageBox::warning(this,tr("Can not add labeling positions"),
				     tr("Could not populate the references "
					"list.\nCreate a labeling postion "
					"to copy from, please."));
		return;
	}
	dialog.setResidueList(residues);
	dialog.exec();
}

void MainWindow::addDistanceBatch()
{
	if(evalsModel.evaluatorsAvailable<EvaluatorPositionSimulation>().empty()) {
		QMessageBox::warning(this,tr("Can not add distances"),
				     tr("Could not populate the labeling "
					"positions list.\nCreate some "
					"labeling postions, please."));
		return;
	}
	if(evalsModel.evaluatorsAvailable<EvaluatorDistance>().empty()) {
		QMessageBox::warning(this,tr("Can not add distances"),
				     tr("Could not populate the references "
					"list.\nCreate a distance "
					"to copy from, please."));
		return;
	}
	BatchDistanceDialog dialog(this,evalsModel);
	dialog.exec();
}

void MainWindow::addEfficiencyBatch()
{
	if(evalsModel.evaluatorsAvailable<EvaluatorPositionSimulation>().empty()) {
		QMessageBox::warning(this,tr("Can not add distances"),
				     tr("Could not populate the labeling "
					"positions list.\nCreate some "
					"labeling postions, please."));
		return;
	}
	BatchFretEfficiencyDialog dialog(this,evalsModel);
	dialog.exec();
}

void MainWindow::getInfromativePairs()
{
	std::vector<FrameDescriptor> frames=trajectoriesModel.frames();
	if(frames.empty()) {
		QMessageBox::warning(this,tr("No structures loaded"),
				     tr("No structures loaded.\n"
					"Load some, please."));
		return;
	}
	std::vector<EvalId> evalIds=_storage.evalIds<EvaluatorFretEfficiency>();
	if(evalIds.empty()) {
		QMessageBox::warning(this,tr("No Efficiencies available"),
				     tr("No Efficiencies are available.\n"
					"Create some, please."));
		return;
	}
	std::vector<std::string> evalNames;
	evalNames.reserve(evalIds.size());
	for (const auto evalId: evalIds) {
		evalNames.push_back(_storage.evalName(evalId));
	}

	const int tasksCount=_storage.tasksPendingCount()+1;
	QProgressDialog progress("Calculating efficiencies...",QString(),0,tasksCount,this);
	progress.setWindowModality(Qt::WindowModal);
	do {
		progress.setValue(tasksCount-_storage.tasksPendingCount());
		if (progress.wasCanceled()) {
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	} while(!_storage.ready());
	progress.setValue(tasksCount);

	Eigen::MatrixXf effs(frames.size(),evalIds.size());
	for (int iFrame=0; iFrame<frames.size(); ++iFrame) {
		const auto& fr=frames[iFrame];
		for(int iEv=0; iEv<evalIds.size(); ++iEv) {
			TaskStorage::Result res=_storage.getResult(fr,evalIds[iEv]);
			auto dRes=std::static_pointer_cast<CalcResult<double>>(res);
			effs(iFrame,iEv)=dRes->get();
		}
	}

	GetInformativePairsDialog dialog(this,frames,effs,evalNames);
	dialog.exec();
}

QString MainWindow::timespan(unsigned seconds)
{
	if (seconds < 61) {
		return QString::number(seconds) + "s";

	} else if (seconds < 3600) {
		return QString::number(seconds / 60.0,'f',2) + "m";

	} else if (seconds < 86400) {
		return QString::number(seconds / 3600.0,'f',2) + "h";
	}

	return QString::number(seconds / 86400) + "d "
			+ QString::number(seconds / 3600 % 24) + "h";
}
