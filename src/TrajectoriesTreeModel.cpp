#include <cassert>
#include <set>
#include <memory>

#include <QFileInfo>
#include <QTimer>
#include <QProgressDialog>
#include <QTime>
#include <QCoreApplication>

#include "TrajectoriesTreeModel.h"
#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"
#include "EvaluatorPositionSimulation.h"


TrajectoriesTreeModel::TrajectoriesTreeModel(const TaskStorage &storage,
					     QObject *parent)
    : QAbstractItemModel(parent), _storage(storage)
{
	connect(
		&_storage, &TaskStorage::evaluatorAdded, this,
		[this](const EvalId &id) { evaluatorAdded(id); },
		Qt::QueuedConnection);
	connect(&_storage, &TaskStorage::evaluatorIsGoingToBeRemoved,
		[this](const EvalId &id) { evaluatorRemove(id); });

	_evaluatePending.setSingleShot(true);
	_evaluatePending.setInterval(1000);
	connect(&_evaluatePending, &QTimer::timeout, [this] {
		if (_evalsPending.empty()) {
			return;
		}
		std::vector<EvalId> evPendingCopy;
		evPendingCopy.swap(_evalsPending);
		_evalsPending.clear();
		QTime runningTime;
		runningTime.start();
		auto pause = _storage.pause();
		// iterate over all frames in trajectories
		for (const MolecularTrajectory &mt : _molTrajs) {
			for (int trajIdx = 0; trajIdx < mt.chunkCount();
			     ++trajIdx) {
				for (int frIdx = 0;
				     frIdx < mt.frameCount(trajIdx); ++frIdx) {
					auto desc =
						mt.descriptor(trajIdx, frIdx);
					_storage.evaluate(desc, evPendingCopy);
					if (runningTime.elapsed() > 20) {
						QCoreApplication::
							processEvents();
						runningTime.restart();
					}
				}
			}
		}
	});
}

QVariant TrajectoriesTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	const TrajectoriesTreeItem *parentItem =
		static_cast<TrajectoriesTreeItem *>(index.internalPointer());

	if (index.column() == 0) {
		return frameName(parentItem, index.row());
	} else {
		if (parentItem->nesting() < 2) {
			return "";
		}
		auto calccol = _columns[index.column() - 1];
		auto frame = frameDescriptor(parentItem, index.row());
		auto string = _storage.getString(frame, calccol.first,
						 calccol.second);
		return QString::fromStdString(string);
	}

	return QVariant();
}

Qt::ItemFlags TrajectoriesTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return QAbstractItemModel::flags(index);
}

QVariant TrajectoriesTreeModel::headerData(int section,
					   Qt::Orientation orientation,
					   int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section == 0) {
			return "structure";
		}
		if (section > 0 && section < int(_columns.size() + 1)) {
			return colName(section - 1);
		}
		return QString("WRONG COLUMN");
	}

	return QVariant();
}

QModelIndex TrajectoriesTreeModel::index(int row, int column,
					 const QModelIndex &parent) const
{
	/*if (!hasIndex(row, column, parent))
	    return QModelIndex();*/

	TrajectoriesTreeItem *parentParentItem;

	if (!parent.isValid())
		parentParentItem = nullptr;
	else
		parentParentItem = static_cast<TrajectoriesTreeItem *>(
			parent.internalPointer());

	const TrajectoriesTreeItem *parentItem =
		childItem(parentParentItem, parent.row());

	return createIndex(row, column,
			   const_cast<TrajectoriesTreeItem *>(parentItem));
}

QModelIndex TrajectoriesTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TrajectoriesTreeItem *parentItem =
		static_cast<TrajectoriesTreeItem *>(index.internalPointer());

	if (parentItem == nullptr || parentItem->nesting() == 0) {
		return QModelIndex();
	}

	assert(parentItem->nesting() > 0);
	assert(parentItem->nesting() < 3);

	if (parentItem->nesting() == 1) {
		return createIndex(parentItem->moltrajIndex, 0, nullptr);
	} else { // if(parentItem->nesting()==2)
		TrajectoriesTreeItem itm{parentItem->moltrajIndex, -1};
		const TrajectoriesTreeItem *pitm =
			&(*(items.emplace(std::move(itm)).first));
		return createIndex(parentItem->trajindex, 0,
				   const_cast<TrajectoriesTreeItem *>(pitm));
	}
}

int TrajectoriesTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	TrajectoriesTreeItem *parentParentItem;
	parentParentItem =
		static_cast<TrajectoriesTreeItem *>(parent.internalPointer());

	if (!parent.isValid()) {
		return _molTrajs.size();
	}
	unsigned parentParentNesting = 0;
	if (parentParentItem != nullptr) {
		parentParentNesting = parentParentItem->nesting();
	}
	if (parentParentNesting == 0) {
		unsigned moltrajIndex = parent.row();
		return _molTrajs[moltrajIndex].chunkCount();
	}
	if (parentParentNesting == 1) {
		unsigned moltrajIndex = parentParentItem->moltrajIndex;
		unsigned trajindex = parent.row();
		return _molTrajs[moltrajIndex].frameCount(trajindex);
	}
	if (parentParentNesting == 2) {
		return 0;
	}
	return 0;
}

int TrajectoriesTreeModel::columnCount(const QModelIndex & /*parent*/) const
{
	return _columns.size() + 1;
}

void TrajectoriesTreeModel::loadTrajectories(
	std::vector<MolecularTrajectory> trajVec)
{
	int numFrames = 0;
	for (const MolecularTrajectory &tr : trajVec) {
		numFrames += tr.totalFrameCount();
	}
	QProgressDialog progress("Submitting jobs...", "Cancel", 0, numFrames);
	progress.setWindowTitle("Submitting jobs...");
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setValue(0);

	// insert Rows
	const int numTraj = trajVec.size();
	const int firstTraj = _molTrajs.size();
	const int lastTraj = _molTrajs.size() + numTraj - 1;
	beginInsertRows(QModelIndex(), firstTraj, lastTraj);
	std::move(trajVec.begin(), trajVec.end(),
		  std::back_inserter(_molTrajs));
	endInsertRows();

	// evaluate
	std::set<EvalId> evIds;
	for (const CalcColumn &cc : _columns) {
		EvalId evId = cc.first;
		evIds.emplace(evId);
	}
	std::vector<EvalId> evVec(evIds.begin(), evIds.end());
	int frLoaded = 0;
	for (int iTr = firstTraj; iTr <= lastTraj; ++iTr) {
		const int numChunks = _molTrajs[iTr].chunkCount();
		for (int iChunk = 0; iChunk < numChunks; ++iChunk) {
			const int numFrames = _molTrajs[iTr].frameCount(iChunk);
			for (int iFr = 0; iFr < numFrames; ++iFr) {
				_storage.evaluate(
					_molTrajs[iTr].descriptor(iChunk, iFr),
					evVec);
				progress.setValue(frLoaded++);
			}
		}
	}
	progress.setValue(numFrames);
}

void TrajectoriesTreeModel::loadPdbs(const QStringList &fileNames)
{

	const int rawSize = fileNames.size();
	QProgressDialog progress("Checking files...", "Cancel", 0, rawSize);
	progress.setWindowTitle("Checking files...");
	progress.setWindowModality(Qt::ApplicationModal);

	// prepare trajectories
	std::vector<MolecularTrajectory> tmpTrajVec;
	for (const QString &fName : fileNames) {
		if (fName.isEmpty()
		    || !fName.endsWith(".pdb", Qt::CaseInsensitive)) {
			continue;
		}
		tmpTrajVec.emplace_back(
			MolecularTrajectory::fromPdb(fName.toStdString()));
		progress.setValue(tmpTrajVec.size());
	}
	progress.setValue(rawSize);

	loadTrajectories(std::move(tmpTrajVec));
}

void TrajectoriesTreeModel::loadDcd(const std::string &topPath,
				    const std::string &trajPath)
{
	async::task<int> numFramesTsk = _storage.numFrames(topPath, trajPath);
	QProgressDialog dlg;
	dlg.setLabelText("Loading trajectory...");
	dlg.setWindowTitle("Loading trajectory...");
	dlg.setRange(0, 0);
	dlg.setWindowModality(Qt::WindowModal);
	dlg.show();
	dlg.setCancelButton(0);
	while (!numFramesTsk.ready()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		QCoreApplication::processEvents();
	}
	dlg.close();
	const int numFrames = numFramesTsk.get();
	if (numFrames == 0) {
		std::cerr << "ERROR! Empty trajectory " + trajPath + "\n"
			  << std::flush;
		return;
	}

	MolecularTrajectory mt =
		MolecularTrajectory::fromDcd(topPath, trajPath, numFrames);
	std::vector<MolecularTrajectory> tmpTrajVec;
	tmpTrajVec.emplace_back(std::move(mt));
	loadTrajectories(std::move(tmpTrajVec));
}

QString TrajectoriesTreeModel::dumpTabSeparatedData() const
{
	QString result;
	QTextStream out(&result, QIODevice::WriteOnly);
	QModelIndex start = index(0, 0);
	QModelIndexList indexes = match(start, Qt::DisplayRole, "*", -1,
					Qt::MatchWildcard | Qt::MatchRecursive);
	int numCols = columnCount();
	out << headerData(0, Qt::Horizontal).toString();
	for (int c = 1; c < numCols; c++) {
		out << "\t" + headerData(c, Qt::Horizontal).toString();
	}
	out << "\n";

	QProgressDialog progress("Generating the table...", "Cancel", 0,
				 indexes.size());
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setWindowTitle("Generating the table...");
	progress.setValue(0);
	for (const auto &idx : indexes) {
		if (progress.wasCanceled()) {
			result.clear();
			break;
		}
		TrajectoriesTreeItem *parentItem;
		parentItem = static_cast<TrajectoriesTreeItem *>(
			idx.internalPointer());
		if (parentItem->nesting() < 2) {
			continue;
		}
		out << data(parent(idx)).toString() + " ";
		out << data(idx.sibling(idx.row(), 0)).toString();
		for (int c = 1; c < numCols; c++) {
			out << "\t";
			out << data(idx.sibling(idx.row(), c)).toString();
		}
		out << "\n";
		progress.setValue(progress.value() + 1);
	}
	progress.setValue(indexes.size());
	return result;
}

const TrajectoriesTreeItem *
TrajectoriesTreeModel::childItem(const TrajectoriesTreeItem *parent,
				 unsigned childRow) const
{
	// assert(parent->nesting()<2);
	if (parent != nullptr && parent->nesting() == 1) {
		TrajectoriesTreeItem itm{parent->moltrajIndex, int(childRow)};
		return &(*(items.emplace(std::move(itm)).first));
	} else { // if(parentNesting==0) || parent!=nullptr
		TrajectoriesTreeItem itm{int(childRow), -1};
		return &(*(items.emplace(std::move(itm)).first));
	}
}

QString TrajectoriesTreeModel::frameName(const TrajectoriesTreeItem *parent,
					 int row) const
{
	QString name;
	switch (parent->nesting()) {
	case 0:
		name = QString::fromStdString(
			*(_molTrajs[row].topologyFileName()));
		name = QFileInfo(name).fileName();
		break;
	case 2:
		return QString("%1").arg(row);
		break;
	case 1:
		name = QString::fromStdString(
			*(_molTrajs[parent->moltrajIndex].trajectoryFileName(
				row)));
		name = QFileInfo(name).fileName();
		break;
	default:
		return QString("???");
	}
	return name;
}

QString TrajectoriesTreeModel::colName(int section) const
{
	const auto &cc = _columns.at(section);
	return QString::fromStdString(
		_storage.getColumnName(cc.first, cc.second));
}

FrameDescriptor
TrajectoriesTreeModel::frameDescriptor(const TrajectoriesTreeItem *parent,
				       int row) const
{
	std::shared_ptr<const std::string> top, traj;
	unsigned frame = -100;
	switch (parent->nesting()) {
	case 0:
		top = _molTrajs[row].topologyFileName();
		traj = _molTrajs[row].trajectoryFileName(0);
		frame = 0;
		break;
	case 1:
		top = _molTrajs[parent->moltrajIndex].topologyFileName();
		traj = _molTrajs[parent->moltrajIndex].trajectoryFileName(row);
		frame = 0;
		break;
	case 2:
		top = _molTrajs[parent->moltrajIndex].topologyFileName();
		traj = _molTrajs[parent->moltrajIndex].trajectoryFileName(
			parent->trajindex);
		frame = row;
		break;
	default:
		top = std::make_shared<const std::string>("?");
		traj = std::make_shared<const std::string>("??");
		frame = -1;
	}
	return FrameDescriptor(top, traj, frame);
}

void TrajectoriesTreeModel::evaluatorAdded(const EvalId &id)
{
	int colCount = _storage.eval(id).columnCount();
	if (colCount == 0) {
		return;
	}
	beginInsertColumns(QModelIndex(), _columns.size() + 1,
			   _columns.size() + colCount);
	for (int i = 0; i < colCount; ++i) {
		_columns.emplace_back(id, i);
	}
	endInsertColumns();
	if (colCount > 0) {
		_evalsPending.push_back(id);
	}
	_evaluatePending.start();
}

void TrajectoriesTreeModel::evaluatorRemove(const EvalId &id)
{
	int colCount = _storage.eval(id).columnCount();
	if (colCount == 0) {
		return;
	}
	int firstCol;
	for (firstCol = 0; _columns[firstCol].first != id; ++firstCol)
		;
	int lastCol;
	for (lastCol = firstCol + 1;
	     _columns[lastCol].first == id && lastCol < _columns.size();
	     ++lastCol)
		;
	--lastCol;
	beginRemoveColumns(QModelIndex(), firstCol + 1, lastCol + 1);
	_columns.erase(_columns.begin() + firstCol,
		       _columns.begin() + lastCol + 1);
	endRemoveColumns();
}
/*
void TrajectoriesTreeModel::updateColumn(int column)
{
	if(column>_columns.size() || column<1) {
		qDebug()<<"ERROR: wrong column number = "<<column;
		return;
	}
	const auto& calccol=_columns[column-1];
	QModelIndex start=index(0,0);
	QModelIndexList indexes = match(start, Qt::DisplayRole, "*", -1,
Qt::MatchWildcard|Qt::MatchRecursive);
	for(const auto& idx:indexes) {
		const TrajectoriesTreeItem *parentItem =
				static_cast<TrajectoriesTreeItem*>(idx.internalPointer());
		if(parentItem->nesting()!=2) {
			continue;
		}
		auto frame=frameDescriptor(parentItem,idx.row());
		//qDebug()<<"data: <"<<idx.data()<<">
row:"<<idx.row()<<"nesting: "<<parentItem->nesting()<<" col:"<<column<<"
frame:"<<QString::fromStdString(frame.trajFileName());
		_storage.getString(frame,calccol.first,calccol.second);
		//emit dataChanged(idx,idx);
	}
}*/
/*	connect(_domainsModel,&DomainTableModel::rowsInserted,
		[&](const QModelIndex &,int from,int to) {
		domainsInserted(from, to);
	});
	connect(_positionsModel,&PositionTableModel::rowsInserted,
		[&](const QModelIndex &,int from,int to) {
		positionsInserted(from, to);
	});
	connect(_distancesModel,&DistanceTableModel::rowsInserted,
		[&](const QModelIndex &,int from,int to) {
		distancesInserted(from, to);
	});
*/
/*
void TrajectoriesTreeModel::domainsInserted(int from, int to)
{
	for(int i=from; i<=to; i++)
	{
		addCalculator(_domainsModel->domain(i));
	}
}

void TrajectoriesTreeModel::positionsInserted(int from, int to)
{
	for(int i=from; i<=to; i++)
	{
		addCalculator(_positionsModel->position(i));
	}
}

void TrajectoriesTreeModel::distancesInserted(int from, int to)
{
	for(int i=from; i<=to; i++)
	{
		addCalculator(_distancesModel->distance(i));
	}
}
void TrajectoriesTreeModel::addCalculator(const
std::shared_ptr<MolecularSystemDomain> domain)
{
	auto
calculator=std::make_shared<EvaluatorTrasformationMatrix>(_storage,domain);

	std::shared_ptr<EvaluatorTrasformationMatrix> firstTmCalc;
	for(const auto& calc:_calculators)
	{
		firstTmCalc=std::dynamic_pointer_cast<EvaluatorTrasformationMatrix>(calc);
		if(firstTmCalc)
		{
			break;
		}
	}

	_calculators.insert(calculator);

	if(firstTmCalc) {
		auto
angleCalc=std::make_shared<EvaluatorEulerAngle>(_storage,firstTmCalc,calculator);
		_calculators.insert(angleCalc);
		beginInsertColumns(QModelIndex(),columnCount(),columnCount()+2);
		_visibleCalculators.push_back(std::make_pair(angleCalc,0));
		_visibleCalculators.push_back(std::make_pair(angleCalc,1));
		_visibleCalculators.push_back(std::make_pair(angleCalc,2));
		endInsertColumns();
	}
}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Position>
position)
{
	auto
calculator=std::make_shared<EvaluatorPositionSimulation>(_storage,position);
	_avCalculators[position->name()]=calculator;
	_calculators.insert(calculator);
	//_visibleCalculators.push_back(std::make_pair(calculator,0));
}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Distance>
distance)
{
	auto av1=_avCalculators.at(distance->position1());
	auto av2=_avCalculators.at(distance->position2());
	auto
calculator=std::make_shared<EvaluatorDistance>(_storage,av1,av2,distance);
	_calculators.insert(calculator);

	beginInsertColumns(QModelIndex(), columnCount(),columnCount());
		_visibleCalculators.push_back(std::make_pair(calculator,0));
	endInsertColumns();

	_distanceCalculators.push_back(calculator);
	_chi2Calc=std::make_shared<EvaluatorChi2>(_storage,_distanceCalculators);
	_calculators.insert(_chi2Calc);
	_visibleCalculators[0]=std::make_pair(_chi2Calc,0);
}
*/
