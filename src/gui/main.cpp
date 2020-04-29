#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QCommandLineParser>
#include <QTimer>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include <fstream>

void dumpException(const std::string &str)
{
	std::ofstream outfile;
	outfile.open("Olga_exception.log", std::ifstream::out);
	if (!outfile.is_open()) {
		return;
	}
	outfile << str;
	outfile.close();
}

int main(int argc, char *argv[])
{
	/*std::cout<<"std::thread::hardware_concurrency()="<<
		   std::thread::hardware_concurrency()<<std::endl;*/
	pteros::Log::instance().logger->set_level(
		spdlog::level::level_enum::warn);
	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	QCommandLineParser parser;
	parser.addOptions({
		{"dir", "directory to load PDB files from", "path"},
		{"traj", "trajectory file", "path"},
		{"top", "topology file", "path"},
		{{"j", "json"},
		 "setting file describing labelig positions and distances",
		 "path"},
		{{"o", "out"}, "results .dat filename", "path"},
		{"savejson", "save generated evaluators", "path"},
		{"numpairs", "number of pairs to select", "integer"},
		{"err", "Efficiency error to assume for pair selection",
		 "float"},
		{"savepairs", "save selected pairs", "path"},
	});
	QCommandLineOption noguiOption("nogui",
				       "Don't show the main window GUI");
	parser.addOption(noguiOption);
	parser.process(a);

	QString settingsFileName = parser.value("j");
	QString pdbsDirPath = parser.value("dir");
	QString trajPath = parser.value("traj");
	QString topPath = parser.value("top");
	QString resultsFileName = parser.value("o");
	bool nogui = parser.isSet(noguiOption);
	QString savejson = parser.value("savejson");
	int numSelPairs = parser.value("numpairs").toInt();
	float err = parser.value("err").toFloat();
	QString pairsPath = parser.value("savepairs");
	MainWindow w(settingsFileName, resultsFileName, trajPath, topPath,
		     pdbsDirPath, savejson, numSelPairs, pairsPath, err);
	try {
		if (!nogui) {
			w.show();
		} else {
			QObject stub;
			QObject::connect(&stub, &QObject::destroyed, &a,
					 &QCoreApplication::quit,
					 Qt::QueuedConnection);
		}
		return a.exec();
	} catch (std::exception &e) {
		dumpException(std::string("std::exception: ") + e.what());
		return 2;
	} catch (boost::exception &e) {
		dumpException("boost::exception: "
			      + boost::diagnostic_information(e));
		return 3;
	} catch (...) {
		dumpException("exception type: unknown");
		return 4;
	}
	return 1;
}
