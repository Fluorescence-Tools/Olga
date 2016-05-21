#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QCommandLineParser>
#include <iostream>

int main(int argc, char *argv[])
{
	std::cout<<"std::thread::hardware_concurrency()="<<
		   std::thread::hardware_concurrency()<<std::endl;

	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	QCommandLineParser parser;
	parser.addOptions({
				  {"dir", "directory to load PDB files from","path"},
				  {{"j","json"},
				   "setting file describing labelig positions and distances", "file"},
				  {{"o","out"},
				   "results .ha4 filename", "file"}
			  });
	QCommandLineOption quietOption("quiet", "quiet");
	parser.addOption(quietOption);
	parser.process(a);
	QString settingsFileName=parser.value("j");
	QString pdbsDirPath=parser.value("dir");
	QString resultsFileName=parser.value("o");
	bool quiet=parser.isSet(quietOption);
	MainWindow w(settingsFileName,pdbsDirPath,resultsFileName);
	if(!quiet) {w.show();}
	return a.exec();
}
