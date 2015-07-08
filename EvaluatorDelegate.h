#ifndef EVALUATORDELEGATE_H
#define EVALUATORDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QToolButton>
#include <QToolBar>
#include <QAction>

#include "EvaluatorsTreeModel.h"

class EvaluatorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit EvaluatorDelegate(EvaluatorsTreeModel &evalModel, QAbstractItemView *parent);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
			      const QModelIndex &index) const;
	void destroyEditor(QWidget * editor, const QModelIndex & index) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model,
			  const QModelIndex &index) const;
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const ;
public Q_SLOTS:
	void cellEntered(const QModelIndex &index);
protected:
	bool editorEvent(QEvent *event, QAbstractItemModel *model,
			 const QStyleOptionViewItem &option,
			 const QModelIndex &index);
Q_SIGNALS:
private:
	QAbstractItemView* _view=0;

	mutable QComboBox comboBox;
	//mutable QComboBox simtypeComboBox;
	mutable QToolBar toolBar;
	mutable QToolBar toolBarPaint;
	void setupToolBar(QToolBar &toolBar);

	//mutable QToolBar toolBarPaint;
	QAction save{QIcon("://icons/document-save.svgz"),"save",this};
	//QAction save2{QIcon("://icons/document-save.svgz"),"save2",this};
	//mutable QToolButton saveEvalBtn;
	//mutable QToolButton saveEvalBtnPaint;
	bool isOneCellInEditMode=false;

	QModelIndex currentEditedCellIndex;

	EvaluatorsTreeModel& _evalModel;
	/*void populate(const std::shared_ptr<const AbstractEvaluator> &eval) const
	{
		const QModelIndex rootIndex=_evalModel.classRowIndex(eval);
		evalComboBox.setRootModelIndex(rootIndex);
	}*/
	const int buttonsType=QVariant::fromValue(ButtonFlags()).userType();
	//const int evalType=QVariant::fromValue(EvalPtr()).userType();
	//const int evalsType=;
	//const int simtypeType=QVariant::fromValue(Position::SimulationType()).userType();
	/*void setSimtype(const QVariant& var) const {
		const auto& type=var.value<Position::SimulationType>();
		simtypeComboBox.setCurrentIndex(static_cast<int>(type));
	}*/
};

#endif // EVALUATORDELEGATE_H
