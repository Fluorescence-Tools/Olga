#include "EvaluatorDelegate.h"
#include "AbstractEvaluator.h"
#include <QLabel>
#include <QPainter>
#include <QEvent>

#include <QAbstractItemView>
#include <QLayout>
#include <QApplication>

EvaluatorDelegate::EvaluatorDelegate(QAbstractItemView *parent)
    : QStyledItemDelegate(parent), _view(parent)
{
	toolBarPaint = new QToolBar(parent);
	setupToolBar(toolBarPaint);
	toolBarPaint->hide();

	_view->setMouseTracking(true);
	connect(_view, SIGNAL(entered(QModelIndex)), this,
		SLOT(cellEntered(QModelIndex)));
}

QWidget *EvaluatorDelegate::createEditor(QWidget *parent,
					 const QStyleOptionViewItem &option,
					 const QModelIndex &index) const
{
	QVariant edata = index.data(Qt::EditRole);
	if (edata.type() == QVariant::StringList) {
		if (index.data().type() == QVariant::Int) {
			auto comboBox = new QComboBox(parent);
			comboBox->addItems(edata.toStringList());
			comboBox->setCurrentIndex(index.data().toInt());
			comboBox->setFrame(false);

			return comboBox;
		} else if (index.data().userType() == intListType) {
			auto checkBoxList = new CheckBoxList(parent);
			checkBoxList->addItems(edata.toStringList());
			checkBoxList->setChecked(
				index.data().value<QList<int>>());
			checkBoxList->setFrame(false);

			return checkBoxList;
		}
	} else if (edata.userType() == vec3dType) {
		auto vec3dedit = new QLineEdit(parent);
		vec3dedit->setFrame(false);
		QRegularExpression re(
			"([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?[;\\s]+){2}"
			"[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?[;\\s]*");
		QRegularExpressionValidator *validator =
			new QRegularExpressionValidator(re);
		vec3dedit->setValidator(validator);
		const Eigen::Vector3d &vec = edata.value<Eigen::Vector3d>();
		QString str =
			QString("%1 %2 %3").arg(vec[0]).arg(vec[1]).arg(vec[2]);
		vec3dedit->setText(str);
		return vec3dedit;
	} else if (edata.userType() == buttonsType) {
		auto toolBar = new QToolBar(parent);
		setupToolBar(toolBar);
		toolBar->setGeometry(option.rect);
		fillToolbar(toolBar, edata.value<ButtonFlags>());
		connect(toolBar, &QToolBar::actionTriggered, this,
			&EvaluatorDelegate::processAction);
		return toolBar;
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void EvaluatorDelegate::setEditorData(QWidget *editor,
				      const QModelIndex &index) const
{
	QVariant data = index.data(Qt::EditRole);
	if (data.type() == QVariant::StringList) {
		QVariant displayData = index.data();
		if (displayData.type() == QVariant::String) {
			auto comboBox = qobject_cast<QComboBox *>(editor);
			comboBox->setCurrentIndex(displayData.toInt());
			return;
		} else if (displayData.userType() == intListType) {
			auto checkBoxList =
				qobject_cast<CheckBoxList *>(editor);
			checkBoxList->setChecked(
				displayData.value<QList<int>>());
			return;
		} else if (data.userType() == buttonsType) {
			auto toolBar = qobject_cast<QToolBar *>(editor);
			fillToolbar(toolBar, data.value<ButtonFlags>());
			return;
		}
	} else if (data.userType() == vec3dType) {
		const Eigen::Vector3d &vec = data.value<Eigen::Vector3d>();
		QString str =
			QString("%1 %2 %3").arg(vec[0]).arg(vec[1]).arg(vec[2]);
		auto vec3dedit = qobject_cast<QLineEdit *>(editor);
		vec3dedit->setText(str);
		return;
	}
	if (data.canConvert<ButtonFlags>()) {
		return;
	}
	QStyledItemDelegate::setEditorData(editor, index);
}

void EvaluatorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
				     const QModelIndex &index) const
{
	QVariant data = index.data(Qt::EditRole);
	if (data.type() == QVariant::StringList) {
		QVariant displayData = index.data();
		if (displayData.type() == QVariant::Int) {
			auto comboBox = qobject_cast<QComboBox *>(editor);
			model->setData(index, comboBox->currentIndex(),
				       Qt::EditRole);
			return;
		} else if (displayData.userType() == intListType) {
			auto checkBoxList =
				qobject_cast<CheckBoxList *>(editor);
			const auto &var =
				QVariant::fromValue(checkBoxList->getChecked());
			model->setData(index, var, Qt::EditRole);
			return;
		}
	} else if (data.userType() == vec3dType) {
		Eigen::Vector3d vec;
		QRegularExpression re("[;\\s]+");
		auto vec3dedit = qobject_cast<QLineEdit *>(editor);
		const QString &str = vec3dedit->text();
		QVector<QStringRef> list =
			str.splitRef(re, QString::SkipEmptyParts);
		for (int i : {0, 1, 2}) {
			vec[i] = list[i].toDouble();
		}
		model->setData(index, QVariant::fromValue(vec), Qt::EditRole);
		return;
	} else if (data.canConvert<ButtonFlags>()) {
		model->setData(index, editor->property("btnFlags"),
			       Qt::EditRole);
		return;
	}
	QStyledItemDelegate::setModelData(editor, model, index);
}

void EvaluatorDelegate::paint(QPainter *painter,
			      const QStyleOptionViewItem &option,
			      const QModelIndex &index) const
{
	QVariant data = index.data();
	if (data.userType() == vec3dType) {
		const Eigen::Vector3d &vec = data.value<Eigen::Vector3d>();
		const auto &str = QString("(%1 %2 %3)")
					  .arg(vec[0])
					  .arg(vec[1])
					  .arg(vec[2]);
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		opt.text = str;
		const QWidget *widget = option.widget;
		QStyle *style =
			widget ? widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter,
				   widget);
	} else if (index.data(Qt::EditRole).type() == QVariant::StringList) {
		QStringList all = index.data(Qt::EditRole).toStringList();
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		if (data.userType() == intListType) {
			QList<int> checked = data.value<QList<int>>();
			QStringList selected;
			for (int i : checked) {
				selected << all.at(i);
			}
			opt.text = selected.join(", ");
			if (selected.empty()) {
				opt.text = "none (click to select)";
			}
		} else if (data.type() == QVariant::Int) {
			if (data.toInt() >= 0) {
				opt.text = all.at(data.toInt());
			} else {
				opt.text = "none (click to select)";
			}
		}
		const QWidget *widget = option.widget;
		QStyle *style =
			widget ? widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter,
				   widget);
	} else if (data.canConvert<ButtonFlags>()) {
		if (option.state == QStyle::State_Selected) {
			painter->fillRect(option.rect,
					  option.palette.highlight());
		}
		toolBarPaint->setGeometry(option.rect);
		fillToolbar(toolBarPaint, data.value<ButtonFlags>());
		QPixmap map = toolBarPaint->grab();
		painter->drawPixmap(option.rect.x(), option.rect.y(), map);
	} else {
		QStyledItemDelegate::paint(painter, option, index);
	}
}

void EvaluatorDelegate::cellEntered(const QModelIndex &index)
{
	QVariant data = index.data(Qt::EditRole);
	if (data.canConvert<ButtonFlags>()) {
		if (_view->isPersistentEditorOpen(persistentEditorIdx)) {
			_view->closePersistentEditor(persistentEditorIdx);
		}
		_view->openPersistentEditor(index);
		persistentEditorIdx = index;
	}
}
bool EvaluatorDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
				    const QStyleOptionViewItem &option,
				    const QModelIndex &index)
{
	return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void EvaluatorDelegate::processAction(QAction *act)
{
	QWidget *editor = qobject_cast<QWidget *>(sender());
	QVariant var;
	var.setValue(actionToFlags(act));
	editor->setProperty("btnFlags", var);
	Q_EMIT commitData(editor);
}

EvaluatorDelegate::ButtonFlags
EvaluatorDelegate::actionToFlags(QAction *act) const
{
	ButtonFlags flgs;
	if (act == &save)
		flgs.save = true;
	else if (act == &del)
		flgs.remove = true;
	else if (act == &duplicate)
		flgs.duplicate = true;
	else
		std::cerr << "ERROR! Unknown action triggered!\n";
	return flgs;
}

void EvaluatorDelegate::setupToolBar(QToolBar *toolBar)
{
	toolBar->setFloatable(false);
	toolBar->setMovable(false);
	toolBar->layout()->setMargin(0);
	toolBar->layout()->setContentsMargins(0, 0, 0, 0);
	toolBar->setStyleSheet(
		"QToolBar { background-color : rgba(255,255,255,0);"
		" color:white; border-color: transparent;}  QToolButton{} ");
	toolBar->setFocusPolicy(Qt::NoFocus);
}

void EvaluatorDelegate::fillToolbar(QToolBar *toolBar,
				    const ButtonFlags &btnFlags) const
{
	QList<QAction *> actions;
	if (btnFlags.save)
		actions.push_back(&save);
	if (btnFlags.remove)
		actions.push_back(&del);
	if (btnFlags.duplicate)
		actions.push_back(&duplicate);
	toolBar->clear();
	toolBar->addActions(actions);
}
