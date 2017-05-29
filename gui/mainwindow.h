#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QItemSelectionModel>
#include <QAbstractItemView>
#include <QLabel>
#include "TrajectoriesTreeModel.h"
#include "TrajectoriesTreeModel.h"
#include "EvaluatorsTreeModel.h"
#include "EvaluatorDelegate.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(const QString json, const QString pdbsDir, const QString ha4Out, const QString selPairs="", QWidget *parent = 0);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private Q_SLOTS:
	void loadStructures();
	void loadStructuresFolder();
	void metropolisSampling();
	void loadEvaluators();
	bool saveJson();
	bool exportData();
	bool exportCylinders();
	bool exportStructures();
	void addEvaluator();

	/*void deleteSelectedDistances();
	void deleteSelectedDomains();
	void deleteSelectedPositions();

	void ShowSystemsContextMenu(const QPoint& pos);
	void ShowDistancesContextMenu(const QPoint& pos);
	void ShowDomainsContextMenu(const QPoint& pos);
	void ShowPositionsContextMenu(const QPoint& pos);
*/
	void copySelectedText(const QItemSelectionModel* selModel) const;
	void pasteText(QAbstractItemView *view) const;

	void expand(const QModelIndex &parentIndex, int first, int last);
	void showAbout();

	void addLpBatch(bool all=false);
	void addDistanceBatch();
	void addEfficiencyBatch(bool all=false);
	void getInfromativePairs();

	void setPaused(bool state=true);

	void showBuffersStats();
	void removeNanEffs();

	void loadResults();
private:
	QString timespan(unsigned seconds);

	void readSettings();
	void writeSettings() const;
	void setupMenus();
	bool eventFilter(QObject* object, QEvent* event);

	QString tabSeparatedData(const QItemSelectionModel *selectionModel) const;
	void loadMolecules(const QStringList& fileNames);

	void loadStructuresFolder(const QString& path);
	void loadEvaluators(const QString& fileName);
	bool exportData(const QString& fileName);
	void autoSelectPairs(const QString& fileName);

private:
	Ui::MainWindow *ui;

	QMenu systemsMenu;
	QMenu distancesMenu;
	QMenu domainsMenu;
	QMenu positionsMenu;

	TaskStorage _storage;
	TrajectoriesTreeModel trajectoriesModel{_storage,this};
	EvaluatorsTreeModel evalsModel{_storage,this};
	EvaluatorDelegate* evaluatorsDelegate;

	//DistanceDelegate distanceDelegate;

	QLabel tasksStatus;

	std::unique_ptr<TaskStorage::Pause> _pause;

};

#endif // MAINWINDOW_H
