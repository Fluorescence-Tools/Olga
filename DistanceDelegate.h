#ifndef DISTANCEDELEGATE_H
#define DISTANCEDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>

class DistanceDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DistanceDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void destroyEditor(QWidget * editor, const QModelIndex & index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void setComboBoxModel(QAbstractItemModel * model) const
    {
        widget.setModel(model);
    }

signals:

private:
    mutable QComboBox widget;

};

#endif // DISTANCEDELEGATE_H
