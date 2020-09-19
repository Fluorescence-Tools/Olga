#ifndef EVALUATORDELEGATE_H
#define EVALUATORDELEGATE_H
#include <cstdint>

#include <Eigen/Dense>

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QToolButton>
#include <QToolBar>
#include <QAction>
#include <QLineEdit>
#include <QVector3D>

#include "CheckBoxList.h"
#include "TaskStorage.h"

class EvaluatorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit EvaluatorDelegate(QAbstractItemView *parent);

	QWidget *createEditor(QWidget *parent,
			      const QStyleOptionViewItem &option,
			      const QModelIndex &index) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model,
			  const QModelIndex &index) const;
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const;
public Q_SLOTS:
	void cellEntered(const QModelIndex &index);

public:
	struct ButtonFlags {
		uint8_t save : 1;
		uint8_t remove : 1;
		uint8_t duplicate : 1;
		ButtonFlags() : save(0), remove(0), duplicate(0)
		{
		}
	};

protected:
	bool editorEvent(QEvent *event, QAbstractItemModel *model,
			 const QStyleOptionViewItem &option,
			 const QModelIndex &index);

private Q_SLOTS:
	void processAction(QAction *act);

private:
	ButtonFlags actionToFlags(QAction *act) const;
	static void setupToolBar(QToolBar *toolBar);
	void fillToolbar(QToolBar *toolBar, const ButtonFlags &btnFlags) const;

private:
	QAbstractItemView *_view = 0;
	QModelIndex persistentEditorIdx;
	mutable QToolBar *toolBarPaint;


	mutable QAction save{QIcon("://icons/document-save.svgz"), "save",
			     this};
	mutable QAction del{QIcon("://icons/edit-delete.svgz"), "delete", this};
	mutable QAction duplicate{QIcon("://icons/edit-copy.svgz"), "duplicate",
				  this};
	bool isOneCellInEditMode = false;


	const int buttonsType = QVariant::fromValue(ButtonFlags()).userType();
	const int intListType = QVariant::fromValue(QList<int>()).userType();
	const int vec3dType = QVariant::fromValue(Eigen::Vector3d()).userType();
};
Q_DECLARE_METATYPE(EvaluatorDelegate::ButtonFlags)
#endif // EVALUATORDELEGATE_H
