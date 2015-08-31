#include <iostream>
#include <QApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include "EvaluatorsTreeModel.h"
#include "TaskStorage.h"
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	QCoreApplication::setApplicationName("av2restraints");
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOptions({
				  {{"p", "pdb"},
				   "PDB file to use for AV simulations"},
				  {{"j","json"},
				   "setting file describing labelig positions and distances"},
				  {"ir","Reference restart file, corresponding to the specified PDB"},
				  {"o","Name for the generated restraints file"},
				  {"or","Name for the generated restart file"}
			  });
	parser.process(a);
	QString settingsFileName=parser.value("j");
	QString pdbFileName=parser.value("p");
	QString restartInFileName=parser.value("ir");
	QString restartOutFileName=parser.value("or");
	QString restraintsFileName=parser.value("o");

	TaskStorage storage;
	EvaluatorsTreeModel evModel(storage);

	QFile settingsFile(settingsFileName);
	if (!settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cout<<"Unable to open file. "<<settingsFile.errorString().toStdString();
		return 1;
	}
	QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
	evModel.loadEvaluators(doc.toVariant().toMap());


	return a.exec();
}
