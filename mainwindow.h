#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QItemSelectionModel>
#include <QLabel>
//#include "SystemsTableModel.h"
#include "TrajectoriesTreeModel.h"
#include "DomainTableModel.h"
#include "PositionTableModel.h"
#include "DistanceTableModel.h"
#include "DistanceDelegate.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void loadStructures();
	void metropolisSampling();
	void loadJson();
	bool saveJson();
	bool exportData();
	bool exportCylinders();
	bool exportStructures();
	bool insertDistances(int position=-1, unsigned num=1);
	bool insertDomains(int position=-1, unsigned num=1);
	bool insertPositions(int position=-1, unsigned num=1);

	void deleteSelectedDistances();
	void deleteSelectedDomains();
	void deleteSelectedPositions();

	void ShowSystemsContextMenu(const QPoint& pos);
	void ShowDistancesContextMenu(const QPoint& pos);
	void ShowDomainsContextMenu(const QPoint& pos);
	void ShowPositionsContextMenu(const QPoint& pos);

	void copySelectedText(const QItemSelectionModel* selModel) const;
	void pasteText(QAbstractItemView *view) const;

	void expand(const QModelIndex &parentIndex, int first, int last);
	void showAbout();

private:
	void readSettings();
	void writeSettings() const;
	void setupMenus();
	bool eventFilter(QObject* object, QEvent* event);

	QString tabSeparatedData(const QItemSelectionModel *selectionModel) const;

private:
	Ui::MainWindow *ui;

	QMenu systemsMenu;
	QMenu distancesMenu;
	QMenu domainsMenu;
	QMenu positionsMenu;

	DomainTableModel domainsModel;
	PositionTableModel positionsModel;
	DistanceTableModel distancesModel;
	//SystemsTableModel systemsModel;
	TrajectoriesTreeModel trajectoriesModel;

	DistanceDelegate distanceDelegate;

	QLabel tasksStatus;

};

#endif // MAINWINDOW_H
