#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	MainWindow w;
	w.show();
	return a.exec();
}
