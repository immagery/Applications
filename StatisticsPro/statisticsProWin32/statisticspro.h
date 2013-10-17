#ifndef STATISTICSPRO_H
#define STATISTICSPRO_H

#include <QtWidgets/QMainWindow>
#include "ui_statisticspro.h"

class statisticsPro : public QMainWindow
{
	Q_OBJECT

public:
	statisticsPro(QWidget *parent = 0);
	~statisticsPro();

private:
	Ui::statisticsProClass ui;
};

#endif // STATISTICSPRO_H
