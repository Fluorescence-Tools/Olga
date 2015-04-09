#include <cassert>

#include <QFileInfo>

#include "TrajectoriesTreeModel.h"
#include "CalculatorTrasformationMatrix.h"
#include "CalculatorEulerAngle.h"
#include "CalculatorPositionSimulation.h"
#include "CalculatorDistance.h"
#include <QTimer>

TrajectoriesTreeModel::
TrajectoriesTreeModel(const DomainTableModel *domainsModel,
		      const PositionTableModel *positionsModel,
		      DistanceTableModel *distancesModel, QObject *parent) :
	QAbstractItemModel(parent),_domainsModel(domainsModel),
	_positionsModel(positionsModel),_distancesModel(distancesModel),
	threadPool(std::thread::hardware_concurrency())
{
	qRegisterMetaType<std::shared_ptr<AbstractCalcResult>>("shared_ptr<AbstractCalcResult>");
	qRegisterMetaType<std::shared_ptr<AbstractCalculator>>("shared_ptr<AbstractCalculator>");
	qRegisterMetaType<FrameDescriptor>("FrameDescriptor");
	connect(_domainsModel,&DomainTableModel::rowsInserted,
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
	connect(this,
		SIGNAL(calculationFinished(FrameDescriptor,std::shared_ptr<AbstractCalculator>,std::shared_ptr<AbstractCalcResult>)),
		this,
		SLOT(updateCache(FrameDescriptor,std::shared_ptr<AbstractCalculator>,std::shared_ptr<AbstractCalcResult>)));
	connect(this,
		SIGNAL(calculationFinished(FrameDescriptor,std::shared_ptr<AbstractCalculator>,std::shared_ptr<AbstractCalcResult>,QModelIndex)),
		this,
		SLOT(updateCache(FrameDescriptor,std::shared_ptr<AbstractCalculator>,std::shared_ptr<AbstractCalcResult>,QModelIndex)));

	_chi2Calc=std::make_shared<CalculatorChi2>(cache,_distanceCalculators);
	_calculators.insert(_chi2Calc);
	_visibleCalculators.push_back(std::make_pair(_chi2Calc,0));
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
		auto calccol=_visibleCalculators[index.column()-1];
		auto cacherow=cache.find(frameDescriptor(parentItem,index.row()));
		if(cacherow==cache.end()) {
			return QString("NA");
		}
		auto iresult=cacherow->second.find(calccol.first);
		if(iresult==cacherow->second.end()) {
			return QString("NA");
		}
		return QString::fromStdString(iresult->second->toString(calccol.second));
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
		if(section>0 && section<int(_visibleCalculators.size()+1))
		{
			return calculatorName(section-1);
		}
		return QString("name");
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

	return createIndex(row, column, (void*)parentItem);
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
		return createIndex(parentItem->trajindex, 0, (void*)pitm);
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
	return _visibleCalculators.size()+1;
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
		recalculateRow(index(_molTrajs.size()-1,0).child(0,0).child(0,0));
		return true;
	}
	return false;
}

bool TrajectoriesTreeModel::
updateCache(const FrameDescriptor desc, const std::shared_ptr<AbstractCalculator> calc,
	    std::shared_ptr<AbstractCalcResult> result)
{

	if(result) {
		cache[desc][calc]=std::move(result);
		return true;
	}
	else {//resubmit task
		QTimer::singleShot(100,this,[this,desc,calc](){appendTask(desc,calc);});
	}
	return false;
}

bool TrajectoriesTreeModel::updateCache(const FrameDescriptor desc,
					const std::shared_ptr<AbstractCalculator> calc,
					std::shared_ptr<AbstractCalcResult> result,
					const QModelIndex index)
{
	if(result) {
		cache[desc][calc]=std::move(result);
		if(index.isValid()) {
			emit dataChanged(index,index);
		}
	}
	else {//resubmit task
		QTimer::singleShot(100,this,[=](){appendTask(desc,calc,index);});
		return false;
	}
	return true;
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
	const auto& calcCol=_visibleCalculators.at(calcNum);
	return QString::fromStdString(calcCol.first->name(calcCol.second));
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

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<MolecularSystemDomain> domain)
{
	auto calculator=std::make_shared<CalculatorTrasformationMatrix>(cache,domain);

	std::shared_ptr<CalculatorTrasformationMatrix> firstTmCalc;
	for(const auto& calc:_calculators)
	{
		firstTmCalc=std::dynamic_pointer_cast<CalculatorTrasformationMatrix>(calc);
		if(firstTmCalc)
		{
			break;
		}
	}

	_calculators.insert(calculator);
	recalculate(calculator);

	if(firstTmCalc) {
		auto  angleCalc=std::make_shared<CalculatorEulerAngle>(cache,firstTmCalc,calculator);
		_calculators.insert(angleCalc);
		beginInsertColumns(QModelIndex(),columnCount(),columnCount()+2);
		_visibleCalculators.push_back(std::make_pair(angleCalc,0));
		_visibleCalculators.push_back(std::make_pair(angleCalc,1));
		_visibleCalculators.push_back(std::make_pair(angleCalc,2));
		endInsertColumns();
		recalculate(angleCalc);
	}

}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Position> position)
{
	auto calculator=std::make_shared<CalculatorPositionSimulation>(cache,position);
	_avCalculators[position->name()]=calculator;
	_calculators.insert(calculator);
	//_visibleCalculators.push_back(std::make_pair(calculator,0));
	recalculate(calculator);
}

void TrajectoriesTreeModel::addCalculator(const std::shared_ptr<Distance> distance)
{
	auto av1=_avCalculators.at(distance->position1());
	auto av2=_avCalculators.at(distance->position2());
	auto calculator=std::make_shared<CalculatorDistance>(cache,av1,av2,distance);
	_calculators.insert(calculator);
	_visibleCalculators.push_back(std::make_pair(calculator,0));
	recalculate(calculator);
	_distanceCalculators.push_back(calculator);
	_chi2Calc=std::make_shared<CalculatorChi2>(cache,_distanceCalculators);
	_calculators.insert(_chi2Calc);
	_visibleCalculators[0]=std::make_pair(_chi2Calc,0);
	recalculate(_chi2Calc);
}
void TrajectoriesTreeModel::recalculate(const std::shared_ptr<AbstractCalculator> calc) const
{
	//TODO: Probably, going through _molTrajs is more efficient
	QModelIndexList Items = this->match(index(0, 0),Qt::DisplayRole,"",
					    -1, Qt::MatchRecursive | Qt::MatchStartsWith | Qt::MatchWrap);
	for(const auto& item:Items)
	{
		auto p=static_cast<TrajectoriesTreeItem*>(item.internalPointer());
		if(p->nesting()==2) {
			FrameDescriptor desc=frameDescriptor(p,item.row());
			appendTask(desc,calc);
		}
	}
}

void TrajectoriesTreeModel::recalculateRow(const QModelIndex &cell) const
{

	FrameDescriptor desc=frameDescriptor(static_cast<TrajectoriesTreeItem*>(cell.internalPointer()),cell.row());
	for(const auto& calc:_calculators)
	{
		appendTask(desc,calc);
	}
	for(int col=1; col<columnCount(); col++)
	{
		//TODO: add index update;
		appendTask(desc,_visibleCalculators[col-1].first,index(cell.row(),col,cell.parent()));
	}
}

std::shared_ptr<AbstractCalcResult>
TrajectoriesTreeModel::calculate(const FrameDescriptor desc,
				 const std::shared_ptr<AbstractCalculator> calc) const
{
	auto it=cachedEntry(desc,calc);
	if(it==cache.end()->second.end()) {
		return calc->calculate(desc);
	}
	return it->second;
}
void TrajectoriesTreeModel::appendTask(const FrameDescriptor desc,
				       const std::shared_ptr<AbstractCalculator> calc,
				       const QModelIndex index) const
{
	threadPool.enqueue([desc,calc,index,this] {
		emit calculationFinished(desc,calc,calculate(desc,calc),index);
	});
}
/*void TrajectoriesTreeModel::appendTask(const QModelIndex index) const
{
	FrameDescriptor desc=frameDescriptor(static_cast<TrajectoriesTreeItem*>(index.internalPointer()),index.row());
	std::shared_ptr<AbstractCalculator> calc=_visibleCalculators[index.column()-1].first;
	appendTask(desc,calc,index);
}*/
void TrajectoriesTreeModel::appendTask(const FrameDescriptor desc,
				       const std::shared_ptr<AbstractCalculator> calc) const
{
	threadPool.enqueue([desc,calc,this]{
		emit calculationFinished(desc,calc,calculate(desc,calc));
	});
}

pteros::System TrajectoriesTreeModel::system(const FrameDescriptor &desc) const
{
	pteros::System sys;
	sys.load(desc.topologyFileName());
	return sys;
}

TrajectoriesTreeModel::ResultCacheCol::iterator
TrajectoriesTreeModel::cachedEntry(const FrameDescriptor &desc, const std::shared_ptr<AbstractCalculator> calc) const
{
	auto cacherow=cache.find(desc);
	if(cacherow==cache.end()) {
		return cache.end()->second.end();
	}
	auto iresult=cacherow->second.find(calc);
	if(iresult==cacherow->second.end()) {
		return cache.end()->second.end();
	}
	return iresult;
}
