#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <iostream>

int main(int argc, char *argv[])
{
	std::cout<<"std::thread::hardware_concurrency()="<<
		   std::thread::hardware_concurrency()<<std::endl;
	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	MainWindow w;
	w.show();
	return a.exec();
}
