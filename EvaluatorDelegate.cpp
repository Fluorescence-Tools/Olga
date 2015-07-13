#include "EvaluatorDelegate.h"
#include "AbstractEvaluator.h"
#include <QLabel>
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QAbstractItemView>
#include <QLayout>
#include <QApplication>

EvaluatorDelegate::EvaluatorDelegate(QAbstractItemView *parent) :
	QStyledItemDelegate(parent),_view(parent)//,_evalModel(evalModel)
{
	//evalComboBox.setModel(&evalModel);
	comboBox.setFrame(false);
	checkBoxList.setFrame(false);

	connect(&save,&QAction::triggered,[this]{
		const QModelIndex& index=toolBar.property("index").value<QModelIndex>();
		ButtonFlags flgs{0,0,0};
		flgs.save=true;
		QVariant var;
		var.setValue(flgs);
		_view->model()->setData(index,var,Qt::EditRole);
	});
	setupToolBar(toolBar);
	setupToolBar(toolBarPaint);
	toolBarPaint.setParent(_view);
	toolBarPaint.hide();

	_view->setMouseTracking(true);
	connect(_view, SIGNAL(entered(QModelIndex)),
		this, SLOT(cellEntered(QModelIndex)));
}

QWidget *EvaluatorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(data.type()==QVariant::StringList) {
		const auto& dispData=index.data();
		if(dispData.type()==QVariant::Int) {//single selection
			comboBox.clear();
			comboBox.addItems(data.toStringList());
			comboBox.setCurrentIndex(dispData.toInt());
			comboBox.setParent(parent);
			return &comboBox;
		} else if(dispData.userType()==intListType) {
			checkBoxList.clear();
			checkBoxList.addItems(data.toStringList());
			checkBoxList.setChecked(dispData.value<QList<int>>());
			checkBoxList.setParent(parent);
			return &checkBoxList;
		}
	}
	if(data.userType()==buttonsType) {
		ButtonFlags btnFlags=data.value<ButtonFlags>();
		if(btnFlags.save) {
			toolBar.setProperty("index",index);
			toolBar.setParent(parent);
			toolBar.setGeometry(option.rect);
			return &toolBar;
		}
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void EvaluatorDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(editor==&comboBox || editor==&toolBar || editor==&checkBoxList) {
		comboBox.setParent(nullptr);
		toolBar.setParent(nullptr);
		toolBarPaint.setParent(nullptr);
		checkBoxList.setParent(nullptr);
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
			comboBox.setCurrentIndex(displayData.toInt());
			return;
		} else if(displayData.userType()==intListType) {
			checkBoxList.setChecked(displayData.value<QList<int>>());
			return;
		}
	}
	if(data.canConvert<ButtonFlags>()) {
		return;
	}
	QStyledItemDelegate::setEditorData(editor, index);
}

void EvaluatorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QVariant data=index.data(Qt::EditRole);
	if(data.type()==QVariant::StringList) {
		QVariant displayData=index.data();
		if(displayData.type()==QVariant::Int) {
			model->setData(index,comboBox.currentIndex(), Qt::EditRole);
			return;
		} else if(displayData.userType()==intListType) {
			const auto& var=QVariant::fromValue(checkBoxList.getChecked());
			model->setData(index,var,Qt::EditRole);
			return;
		}
	}
	if(data.canConvert<ButtonFlags>()) {
		return;
	}
	QStyledItemDelegate::setModelData(editor, model, index);
}

void EvaluatorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QVariant data=index.data();
	if(index.data(Qt::EditRole).type()==QVariant::StringList) {
		QStringList all=index.data(Qt::EditRole).toStringList();
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		if (data.userType()==intListType) {
			QList<int> checked=data.value<QList<int>>();
			QStringList selected;
			for(int i:checked) {
				selected<<all.at(i);
			}
			opt.text=selected.join(", ");
		} else if(data.type()==QVariant::Int) {
			if(data.toInt()>=0) {
				opt.text=all.at(data.toInt());
			} else {
				opt.text="";
			}
		}
		const QWidget *widget = option.widget;
		QStyle *style = widget ? widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
	} else if(data.canConvert<ButtonFlags>()) {
		if (option.state == QStyle::State_Selected) {
			painter->fillRect(option.rect, option.palette.highlight());
		}
		toolBarPaint.setParent(_view);
		toolBarPaint.setGeometry(option.rect);
		QPixmap map = toolBarPaint.grab();
		painter->drawPixmap(option.rect.x(),option.rect.y(),map);
	}
	else {
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
	toolBar.setStyleSheet("QToolBar { background-color : rgba(255,255,255,0);"
			      " color:white; border-color: transparent;}  QToolButton{} ");
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
