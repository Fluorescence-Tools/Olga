#ifndef CHECKBOXLIST_H
#define CHECKBOXLIST_H
#include <QComboBox>
class CheckBoxList : public QComboBox
{
	Q_OBJECT

public:
	CheckBoxList(QWidget *widget = 0);
	virtual ~CheckBoxList();
	bool eventFilter(QObject *object, QEvent *event);
	virtual void paintEvent(QPaintEvent *);
	void setChecked(const QList<int> &list);
	QList<int> getChecked() const;
	// void SetDisplayText(QString text);
	// QString GetDisplayText() const;

private:
	QString m_DisplayText;
};

#endif // CHECKBOXLIST_H
