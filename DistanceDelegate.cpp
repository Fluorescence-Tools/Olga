#include "DistanceDelegate.h"

DistanceDelegate::DistanceDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
    widget.addItem("");
}

QWidget *DistanceDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column()==1 || index.column()==2)
    {
        widget.setParent(parent);
        widget.setFrame(false);
        return &widget;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void DistanceDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    if(index.column()==1 || index.column()==2)
    {
        return;
    }
    QStyledItemDelegate::destroyEditor(editor,index);
}

void DistanceDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(index.column()==1 || index.column()==2)
    {
        QString value=index.model()->data(index,Qt::EditRole).toString();
        widget.setCurrentText(value);
        return;
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void DistanceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(index.column()==1 || index.column()==2)
    {
        model->setData(index, widget.currentText(), Qt::EditRole);
        return;
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}
