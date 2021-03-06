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


namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(const QString json, const QString csvOut,
			    const QString trajPath, const QString topPath,
			    const QString pdbsDir, const QString dumpJsonPath,
			    int numSelPairs, const QString selPairsPath,
			    float err, QWidget *parent = nullptr);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private Q_SLOTS:
	void loadPdbsDialog();
	void loadTrajDialog();
	void loadStructuresFolder();
	void metropolisSampling();
	void loadEvaluators();
	bool saveJson();
	bool exportData();
	bool exportStructures();
	void addEvaluator();

	void copySelectedText(const QItemSelectionModel *selModel) const;
	void pasteText(QAbstractItemView *view) const;

	void expand(const QModelIndex &parentIndex, int first, int last);
	void showAbout();
	void showDocumentation();

	void addLpBatch(bool all = false);
	void addDistanceBatch();
	void addEfficiencyBatch(bool all = false);
	void getInfromativePairs();
	void getInfromativePairs(int numPairs, float err, const QString &path);
	void testSelectionExpression();

	void setPaused(bool state = true);

	void showBuffersStats();
	void removeNanEffs();

	void loadResults();

	/*
	void deleteSelectedDistances();
	void deleteSelectedDomains();
	void deleteSelectedPositions();

	void ShowSystemsContextMenu(const QPoint& pos);
	void ShowDistancesContextMenu(const QPoint& pos);
	void ShowDomainsContextMenu(const QPoint& pos);
	void ShowPositionsContextMenu(const QPoint& pos);
	*/
private:
	bool saveJson(const QString &fileName);
	QString timespan(unsigned seconds);

	void readSettings();
	void writeSettings() const;
	void setupMenus();
	bool eventFilter(QObject *object, QEvent *event);

	QString
	tabSeparatedData(const QItemSelectionModel *selectionModel) const;
	void loadPdbs(const QStringList &fileNames);

	void loadStructuresFolder(const QString &path);
	void loadEvaluators(const QString &fileName);
	bool exportData(const QString &fileName);
	void autoSelectPairs(int numPairs, float err, const QString &outPath);
	void waitReady() const;
	void waitEvaluators() const;

private:
	Ui::MainWindow *ui;

	QMenu systemsMenu;
	QMenu distancesMenu;
	QMenu domainsMenu;
	QMenu positionsMenu;

	TaskStorage _storage;
	TrajectoriesTreeModel trajectoriesModel{_storage, this};
	EvaluatorsTreeModel evalsModel{_storage, this};
	EvaluatorDelegate *evaluatorsDelegate;

	// DistanceDelegate distanceDelegate;

	QLabel tasksStatus;

	std::unique_ptr<TaskStorage::Pause> _pause;
};

#endif // MAINWINDOW_H
