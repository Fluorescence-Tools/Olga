#include <cassert>

#include <QFileInfo>

#include "TrajectoriesTreeModel.h"
#include "EvaluatorTrasformationMatrix.h"
#include "EvaluatorEulerAngle.h"
#include "EvaluatorPositionSimulation.h"
//#include "CalculatorPositionSimulation.h"
//#include "CalculatorDistance.h"
#include <QTimer>

TrajectoriesTreeModel::
TrajectoriesTreeModel(const TaskStorage &storage, QObject *parent) :
	QAbstractItemModel(parent),_storage(storage)
{
	connect(&_storage,&TaskStorage::evaluatorAdded,
		[this](int i){evaluatorAdded(i);});
}

QVariant TrajectoriesTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	const TrajectoriesTreeItem *parentItem =
			static_cast<TrajectoriesTreeItem*>(index.internalPointer());

	if(index.column()==0) {
		return frameName(parentItem,index.row());
	}
	else {
		if(parentItem->nesting()<2) {
			return "";
		}
		auto calccol=_columns[index.column()-1];
		auto frame=frameDescriptor(parentItem,index.row());
		auto string=_storage.getString(frame,calccol.first,calccol.second);
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

QVariant TrajectoriesTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		if(section==0) {
			return "structure";
		}
		if(section>0 && section<int(_columns.size()+1))
		{
			return calculatorName(section-1);
		}
		return QString("WRONG COLUMN");
	}

	return QVariant();
}

QModelIndex TrajectoriesTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	/*if (!hasIndex(row, column, parent))
	    return QModelIndex();*/

	TrajectoriesTreeItem *parentParentItem;

	if (!parent.isValid())
		parentParentItem = nullptr;
	else
		parentParentItem = static_cast<TrajectoriesTreeItem*>(parent.internalPointer());

	const TrajectoriesTreeItem *parentItem=childItem(parentParentItem,parent.row());

	return createIndex(row, column, const_cast<TrajectoriesTreeItem*>(parentItem));
}

QModelIndex TrajectoriesTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TrajectoriesTreeItem *parentItem = static_cast<TrajectoriesTreeItem*>(index.internalPointer());

	if (parentItem == nullptr || parentItem->nesting()==0) {
		return QModelIndex();
	}

	assert(parentItem->nesting()>0);
	assert(parentItem->nesting()<3);

	if(parentItem->nesting()==1) {
		return createIndex(parentItem->moltrajIndex, 0, nullptr);
	}
	else{ //if(parentItem->nesting()==2)
		TrajectoriesTreeItem itm {parentItem->moltrajIndex,-1};
		const TrajectoriesTreeItem *pitm=&(*(items.emplace(std::move(itm)).first));
		return createIndex(parentItem->trajindex, 0, const_cast<TrajectoriesTreeItem*>(pitm));
	}
}

int TrajectoriesTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	TrajectoriesTreeItem *parentParentItem;
	parentParentItem = static_cast<TrajectoriesTreeItem*>(parent.internalPointer());

	if(!parent.isValid())
	{
		return _molTrajs.size();
	}
	unsigned parentParentNesting=0;
	if(parentParentItem!=nullptr) {
		parentParentNesting=parentParentItem->nesting();
	}
	if(parentParentNesting==0)
	{
		unsigned moltrajIndex=parent.row();
		return _molTrajs[moltrajIndex].trajCount();
	}
	if(parentParentNesting==1){
		unsigned moltrajIndex=parentParentItem->moltrajIndex;
		unsigned trajindex=parent.row();
		return _molTrajs[moltrajIndex].frameCount(trajindex);
	}
	if(parentParentNesting==2){
		return 0;
	}
	return 0;
}

int TrajectoriesTreeModel::columnCount(const QModelIndex &/*parent*/) const
{
	return _columns.size()+1;
}

bool TrajectoriesTreeModel::loadSystem(const QString &fileName)
{
	MolecularTrajectory tmpTrj;
	tmpTrj.setTopology(fileName.toStdString());
	if(tmpTrj.loadFrame(fileName.toStdString()))
	{
		beginInsertRows(QModelIndex(),_molTrajs.size(),_molTrajs.size());
		_molTrajs.push_back(tmpTrj);
		endInsertRows();
		return true;
	}
	return false;
}

QByteArray TrajectoriesTreeModel::tabSeparatedData() const
{
	QString tsv;
	QModelIndex start=index(0,0);
	QModelIndexList indexes = match(start, Qt::DisplayRole, "*", -1,
					Qt::MatchWildcard|Qt::MatchRecursive);
	int numCols=columnCount();
	for(int c=0;c<numCols; c++)
	{
		tsv+=headerData(c,Qt::Horizontal).toString()+"\t";
	}
	tsv+="\n";
	for(const auto& idx:indexes)
	{
		TrajectoriesTreeItem *parentItem;
		parentItem = static_cast<TrajectoriesTreeItem*>(idx.internalPointer());
		if(parentItem->nesting()<2) {
			continue;
		}
		tsv+=data(parent(idx)).toString()+" ";
		for(int c=0;c<numCols; c++)
		{
			tsv+=data(idx.sibling(idx.row(),c)).toString()+"\t";
		}
		tsv+="\n";
	}
	return tsv.toUtf8();
}

const TrajectoriesTreeItem* TrajectoriesTreeModel::childItem(const TrajectoriesTreeItem *parent, unsigned childRow) const
{
	//assert(parent->nesting()<2);
	if(parent!=nullptr && parent->nesting()==1){
		TrajectoriesTreeItem itm {parent->moltrajIndex,int(childRow)};
		return &(*(items.emplace(std::move(itm)).first));
	}
	else {//if(parentNesting==0) || parent!=nullptr
		TrajectoriesTreeItem itm {int(childRow),-1};
		return &(*(items.emplace(std::move(itm)).first));
	}
}

QString TrajectoriesTreeModel::frameName(const TrajectoriesTreeItem *parent, int row) const
{
	QString name;
	switch (parent->nesting()){
	case 0:
		name=QString::fromStdString(_molTrajs[row].topologyFileName());
		name=QFileInfo(name).fileName();
		break;
	case 2:
		return QString("#%1").arg(row);
		break;
	case 1:
		name=QString::fromStdString(_molTrajs[parent->moltrajIndex].trajectoryFileName(row));
		name=QFileInfo(name).fileName();
		break;
	default:
		return QString("???");
	}
	return name;
}

QString TrajectoriesTreeModel::calculatorName(int calcNum) const
{
	const auto& cc=_columns.at(calcNum);
	return QString::fromStdString(_storage.getColumnName(cc.first,cc.second));
}

FrameDescriptor TrajectoriesTreeModel::frameDescriptor(const TrajectoriesTreeItem *parent, int row) const
{
	std::string top,traj;
	unsigned frame=-100;
	switch (parent->nesting()){
	case 0:
		top=_molTrajs[row].topologyFileName();
		traj=_molTrajs[row].trajectoryFileName(0);
		frame=0;
		break;
	case 1:
		top=_molTrajs[parent->moltrajIndex].topologyFileName();
		traj=_molTrajs[parent->moltrajIndex].trajectoryFileName(row);
		frame=0;
		break;
	case 2:
		top=_molTrajs[parent->moltrajIndex].topologyFileName();
		traj=_molTrajs[parent->moltrajIndex].trajectoryFileName(row);
		frame=row;
		break;
	default:
		top="?";
		traj="??";
		frame=-1;
	}
	return FrameDescriptor(top,traj,frame);
}

void TrajectoriesTreeModel::updateColumn(int column)
{
	//TODO:approach should be reconsidered. A hack.
	const auto& calccol=_columns[column];
	QModelIndex start=index(0,column);
	QModelIndexList indexes = match(start, Qt::DisplayRole, "*", -1, Qt::MatchWildcard|Qt::MatchRecursive);
	for(const auto& idx:indexes)
	{
		const TrajectoriesTreeItem *parentItem =
				static_cast<TrajectoriesTreeItem*>(idx.internalPointer());
		auto frame=frameDescriptor(parentItem,idx.row());
		_storage.getString(frame,calccol.first,calccol.second);
		//emit dataChanged(idx,idx);
	}

}

void TrajectoriesTreeModel::evaluatorAdded(int ev)
{
	int colCount=_storage.getColumnCount(ev);
	for(int i=0; i<colCount; ++i) {
		_columns.emplace_back(ev,i);
	}
}
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
void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<MolecularSystemDomain> domain)
{
	auto calculator=std::make_shared<EvaluatorTrasformationMatrix>(_storage,domain);

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
		auto  angleCalc=std::make_shared<EvaluatorEulerAngle>(_storage,firstTmCalc,calculator);
		_calculators.insert(angleCalc);
		beginInsertColumns(QModelIndex(),columnCount(),columnCount()+2);
		_visibleCalculators.push_back(std::make_pair(angleCalc,0));
		_visibleCalculators.push_back(std::make_pair(angleCalc,1));
		_visibleCalculators.push_back(std::make_pair(angleCalc,2));
		endInsertColumns();
	}
}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Position> position)
{
	auto calculator=std::make_shared<EvaluatorPositionSimulation>(_storage,position);
	_avCalculators[position->name()]=calculator;
	_calculators.insert(calculator);
	//_visibleCalculators.push_back(std::make_pair(calculator,0));
}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Distance> distance)
{
	auto av1=_avCalculators.at(distance->position1());
	auto av2=_avCalculators.at(distance->position2());
	auto calculator=std::make_shared<EvaluatorDistance>(_storage,av1,av2,distance);
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
