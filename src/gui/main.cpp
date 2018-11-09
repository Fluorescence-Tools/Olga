#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QCommandLineParser>
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
	        {{"j", "json"},
	         "setting file describing labelig positions and distances",
	         "file"},
	        {{"o", "out"}, "results .dat filename", "file"},
	        {"selectpairs", "source PDB file", "path"},
	        {"savejson", "save generated evaluators", "path"},
	});
	QCommandLineOption quietOption("quiet", "quiet");
	parser.addOption(quietOption);
	parser.process(a);
	QString settingsFileName = parser.value("j");
	QString pdbsDirPath = parser.value("dir");
	QString resultsFileName = parser.value("o");
	bool quiet = parser.isSet(quietOption);
	QString selectpairs = parser.value("selectpairs");
	QString savejson = parser.value("savejson");
	MainWindow w(settingsFileName, pdbsDirPath, resultsFileName,
	             selectpairs, savejson);
	try {
		if (!quiet) {
			w.show();
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
