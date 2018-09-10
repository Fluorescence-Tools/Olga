#include <QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QDir>

#include "TaskStorage.h"
#include "CalcResult.h"
#include "AbstractEvaluator.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QCommandLineParser parser;
	parser.addOptions({
				  {{"j","json"},
				   "setting file describing labelig positions, distances, etc", "file"},
				  {"pdb","PDB file","path"},
				  {"dir","a directory of PDB files","path"}
			  });
	parser.process(a);
	const QString jsonPath=parser.value("j");
	//const QString resultsPath=parser.value("o");
	const QString pdbPath=parser.value("pdb");
	const QString dirPath=parser.value("dir");

	if(pdbPath.isEmpty() && dirPath.isEmpty()) {
		std::cerr<<"Neither --pdb nor --dir where specified. Quitting."<<std::endl;
		return 3;
	}

	//read json
	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cerr<<"Unable to open Json file\n"
			<<jsonFile.errorString().toStdString()<<"\n"
		       <<jsonPath.toStdString()<<std::endl;
		return 1;
	}
	QJsonParseError jsonErr;
	QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll(),&jsonErr);
	if (doc.isNull()) {
		std::cerr<<"Invalid json file"<<std::endl;
		std::cerr<<jsonErr.errorString().toStdString()+" ("
			+std::to_string(jsonErr.offset)+")"<<std::endl;
		return 2;
	}
	QVariantMap evalsData=doc.toVariant().toMap();
	//create evals
	TaskStorage storage;
	storage.loadEvaluators(evalsData);

	std::vector<FrameDescriptor> frames;
	if(!pdbPath.isEmpty()) {
		auto pdbFnamePtr=std::make_shared<const std::string>(pdbPath.toStdString());
		frames.emplace_back(pdbFnamePtr,pdbFnamePtr);
	}
	if(!dirPath.isEmpty()) {
		QDir dir(dirPath);
		for (const QString& path:dir.entryList({"*.pdb"})) {
			auto pdbFnamePtr=std::make_shared<const std::string>((dirPath+'/'+path).toStdString());
			frames.emplace_back(pdbFnamePtr,pdbFnamePtr);
		}
	}
	//submit jobs
	for(const auto& frame:frames) {
		storage.evaluate(frame);
	}

	//wait
	while(! storage.ready()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	//print results
	using std::string;
	if(frames.size()==1) {
		std::unordered_map<string, string> res=storage.getStrings(frames[0]);
		for (const auto& pair: res) {
			std::cout<<pair.first<<'\t'<<pair.second<<'\n';
		}
		std::cout<<std::flush;
		return 0;
	} else if(frames.size()>1) {
		std::vector<string> evNames;
		std::unordered_map<string, string> res=storage.getStrings(frames[0]);
		std::cout<<"structure";
		for(const auto& pair: res) {
			evNames.push_back(pair.first);
			std::cout<<'\t'<<pair.first;
		}
		std::cout<<'\n';
		for(const FrameDescriptor& frame: frames) {
			res=storage.getStrings(frame);
			std::cout<<frame.fullName();
			for(const auto& name: evNames) {
				std::cout<<'\t'<<res.at(name);
			}
			std::cout<<'\n';
		}
		std::cout<<std::flush;
		return 0;
	}
	//return a.exec();
	return 0;
}
