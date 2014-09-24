#ifndef AIRVIEWER_H
#define AIRVIEWER_H

#include <QtWidgets/QMainWindow>
#include "ui_airviewer.h"

class AirViewer : public QMainWindow
{
	Q_OBJECT

public:
	AirViewer(QWidget *parent = 0);
	~AirViewer();

private:
	Ui::AirViewerClass ui;
};

#endif // AIRVIEWER_H
