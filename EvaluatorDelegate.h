#ifndef EVALUATORDELEGATE_H
#define EVALUATORDELEGATE_H
#include <cstdint>

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QToolButton>
#include <QToolBar>
#include <QAction>
#include <QLineEdit>
#include <QVector3D>
#include <QDebug>

#include "CheckBoxList.h"

class EvaluatorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit EvaluatorDelegate(QAbstractItemView *parent);

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
public:
	struct ButtonFlags {
		uint8_t save : 1;
		uint8_t remove : 1;
		uint8_t duplicate : 1;
		ButtonFlags():save(0),remove(0),duplicate(0){}
	};
protected:
	bool editorEvent(QEvent *event, QAbstractItemModel *model,
			 const QStyleOptionViewItem &option,
			 const QModelIndex &index);
Q_SIGNALS:
private:
	QAbstractItemView* _view=0;
	mutable QComboBox comboBox;
	mutable CheckBoxList checkBoxList;
	mutable QLineEdit vec3dedit;
	mutable QToolBar toolBar;
	mutable QToolBar toolBarPaint;

	void setupToolBar(QToolBar &toolBar);

	mutable QAction save{QIcon("://icons/document-save.svgz"),"save",this};
	mutable QAction del{QIcon("://icons/edit-delete.svgz"),"delete",this};
	mutable QAction duplicate{QIcon("://icons/edit-copy.svgz"),"duplicate",this};
	void updateToolBar(QToolBar &toolBar, const ButtonFlags& btnFlags) const {
		//save.setEnabled(btnFlags.save);
		//del.setEnabled(btnFlags.remove);
		//duplicate.setEnabled(btnFlags.duplicate);
		QList<QAction*> actions;
		if(btnFlags.save) {
			actions.push_back(&save);
		}
		if(btnFlags.remove) {
			actions.push_back(&del);
		}
		if(btnFlags.duplicate) {
			actions.push_back(&duplicate);
		}
		toolBar.clear();
		toolBar.addActions(actions);
	}
	bool isOneCellInEditMode=false;

	QModelIndex currentEditedCellIndex;

	//EvaluatorsTreeModel& _evalModel;
	/*void populate(const std::shared_ptr<const AbstractEvaluator> &eval) const
	{
		const QModelIndex rootIndex=_evalModel.classRowIndex(eval);
		evalComboBox.setRootModelIndex(rootIndex);
	}*/
	const int buttonsType=QVariant::fromValue(ButtonFlags()).userType();
	const int intListType=QVariant::fromValue(QList<int>()).userType();
	const int vec3dType=QVariant::fromValue(QVector3D()).userType();
	//const int evalType=QVariant::fromValue(EvalPtr()).userType();
	//const int evalsType=;
	//const int simtypeType=QVariant::fromValue(Position::SimulationType()).userType();
	/*void setSimtype(const QVariant& var) const {
		const auto& type=var.value<Position::SimulationType>();
		simtypeComboBox.setCurrentIndex(static_cast<int>(type));
	}*/
};
Q_DECLARE_METATYPE(EvaluatorDelegate::ButtonFlags)
#endif // EVALUATORDELEGATE_H
