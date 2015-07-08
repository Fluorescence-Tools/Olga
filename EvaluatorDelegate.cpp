#include "EvaluatorDelegate.h"
#include "AbstractEvaluator.h"
#include <QLabel>
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QAbstractItemView>
#include <QLayout>

EvaluatorDelegate::EvaluatorDelegate(EvaluatorsTreeModel &evalModel,
				     QAbstractItemView *parent) :
	QStyledItemDelegate(parent),_evalModel(evalModel),_view(parent)
{
	//evalComboBox.setModel(&evalModel);
	comboBox.setFrame(false);

	connect(&save,&QAction::triggered,[this]{
		const QModelIndex& index=toolBar.property("index").value<QModelIndex>();
		_evalModel.saveEvaluator(index);
	});
	setupToolBar(toolBar);
	setupToolBar(toolBarPaint);
	toolBarPaint.setParent(_view);
	toolBarPaint.hide();

	/*saveEvalBtn.setFocusPolicy(Qt::NoFocus);
	saveEvalBtn.setIcon(QIcon("://icons/document-save.svgz"));
	saveEvalBtn.hide();
	connect(&saveEvalBtn,&QToolButton::pressed,[this]{
		const QModelIndex& index=saveEvalBtn.property("index").value<QModelIndex>();
		_evalModel.saveEvaluator(index);
	});


	saveEvalBtnPaint.setIcon(QIcon("://icons/document-save.svgz"));
	saveEvalBtnPaint.hide();
	*/

	_view->setMouseTracking(true);
	connect(_view, SIGNAL(entered(QModelIndex)),
		this, SLOT(cellEntered(QModelIndex)));

	qDebug()<<"CBox:"<<&comboBox;
//	qDebug()<<"simCBox"<<&simtypeComboBox;
//	qDebug()<<"saveEvalBtn"<<&saveEvalBtn;
	qDebug()<<"_view"<<_view;
}

QWidget *EvaluatorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(data.type()==QVariant::StringList) {
		if(index.data().type()==QVariant::String) {//single selection
			comboBox.clear();
			comboBox.addItems(data.toStringList());
			comboBox.setParent(parent);
			return &comboBox;
		} else {// if(index.data().type()==QVariant::StringList) //
		}
	}
	/*if(data.userType()==evalType) {
		const EvalPtr& eval=data.value<EvalPtr>();
		evalComboBox.setParent(parent);
		populate(eval);
		return &evalComboBox;
	}*/
	if(data.userType()==buttonsType) {
		ButtonFlags btnFlags=data.value<ButtonFlags>();
		if(btnFlags.save) {
			/*saveEvalBtn.setParent(parent);
			saveEvalBtn.setProperty("index",index);
			auto rect=option.rect;
			rect.setWidth(rect.height());
			saveEvalBtn.setGeometry(rect);
			return &saveEvalBtn;*/
			//toolBar.setGeometry(option.rect);
			toolBar.setProperty("index",index);
			toolBar.setParent(parent);
			toolBar.setGeometry(option.rect);
			return &toolBar;
		}
	}
	/*if(data.userType()==simtypeType) {
		simtypeComboBox.setParent(parent);
		setSimtype(data);
		return &simtypeComboBox;
	}*/
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void EvaluatorDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(editor==&comboBox || editor==&toolBar) {
		//simtypeComboBox.setParent(nullptr);
		comboBox.setParent(nullptr);
		toolBar.setParent(nullptr);
		toolBarPaint.setParent(nullptr);
		return;
	}
	/*if(data.userType()==evalType || data.userType()==buttonsType ||
	   data.userType()==simtypeType) {
	}*/
	//qDebug()<<"destroying:"<<editor;
	QStyledItemDelegate::destroyEditor(editor,index);
}

void EvaluatorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(data.type()==QVariant::StringList) {
		QVariant displayData=index.data();
		if(displayData.type()==QVariant::String) {
			comboBox.setCurrentText(displayData.toString());
			return;
		}
	}
	/*if(data.userType()==evalType) {
		QString name=QString::fromStdString(data.value<EvalPtr>()->name());
		comboBox.setCurrentText(name);
		return;
	}*/
	if(data.canConvert<ButtonFlags>()) {
		return;
	}
	/*if(data.userType()==simtypeType) {
		setSimtype(data);
		return;
	}*/
	QStyledItemDelegate::setEditorData(editor, index);
}

void EvaluatorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	/*
	if(data.canConvert<EvalPtr>()) {
		auto oldEval=data.value<EvalPtr>();
		auto newEval=_evalModel.eval(oldEval,evalComboBox.currentIndex());
		if(newEval) {
			model->setData(index,QVariant::fromValue(newEval), Qt::EditRole);
		}
		return;
	}*/
	if(data.type()==QVariant::StringList) {
		QVariant displayData=index.data();
		if(displayData.type()==QVariant::String) {
			model->setData(index,comboBox.currentText(), Qt::EditRole);
			return;
		}
	}
	if(data.canConvert<ButtonFlags>()) {
		return;
	}
	/*if(data.userType()==simtypeType) {
		auto type=static_cast<Position::SimulationType>(simtypeComboBox.currentIndex());
		model->setData(index,QVariant::fromValue(type),Qt::EditRole);
		return;
	}*/
	QStyledItemDelegate::setModelData(editor, model, index);
}

void EvaluatorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(data.canConvert<ButtonFlags>()) {
		if (option.state == QStyle::State_Selected) {
			painter->fillRect(option.rect, option.palette.highlight());
		}
		toolBarPaint.setParent(_view);
		toolBarPaint.setGeometry(option.rect);
		QPixmap map = toolBarPaint.grab();
		painter->drawPixmap(option.rect.x(),option.rect.y(),map);
	} else {
		QStyledItemDelegate::paint(painter,option, index);
	}
}

void EvaluatorDelegate::cellEntered(const QModelIndex &index)
{
	QVariant data=index.data(Qt::EditRole);
	if(data.canConvert<ButtonFlags>()) {
		if(isOneCellInEditMode) {
			toolBar.hide();
			_view->closePersistentEditor(currentEditedCellIndex);
		}
		_view->openPersistentEditor(index);
		isOneCellInEditMode = true;
		currentEditedCellIndex = index;
	} else {
		if(isOneCellInEditMode) {
			isOneCellInEditMode = false;
			_view->closePersistentEditor(currentEditedCellIndex);
		}
	}
}
bool EvaluatorDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
				    const QStyleOptionViewItem &option,
				    const QModelIndex &index)
{
	//qDebug() << event->type();
	return QStyledItemDelegate::editorEvent(event,model,option,index);
}

void EvaluatorDelegate::setupToolBar(QToolBar &toolBar) {
	toolBar.insertAction(0,&save);
	toolBar.setFloatable(false);
	toolBar.setMovable(false);
	toolBar.layout()->setMargin(0);
	toolBar.layout()->setContentsMargins(0,0,0,0);
	toolBar.setStyleSheet("QToolBar { background-color : rgba(255,255,255,0) ; color:white; border-color: transparent;}  QToolButton{} ");
	toolBar.setFocusPolicy(Qt::NoFocus);
}

/*
void EvaluatorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data().canConvert<std::shared_ptr<AbstractEvaluator>>()) {
		QString name=QString::fromStdString(index.data().value<std::shared_ptr<AbstractEvaluator>>()->name());

		if (option.state & QStyle::State_Selected)
		    painter->fillRect(option.rect, option.palette.highlight());


		painter->save();
		painter->setBrush(option.palette.foreground());
		painter->translate(option.rect.x(), option.rect.y());
		painter->drawEllipse(0,0, 10,10);
		painter->restore();
	    } else {
		QStyledItemDelegate::paint(painter, option, index);
	    }
}
*/
