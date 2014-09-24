#include "airviewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	AirViewer w;
	w.show();
	return a.exec();
}
