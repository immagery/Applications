#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets\qmainwindow.h>
#include <QtWidgets\qapplication.h>
#include <QtWidgets\QLineEdit>

//#define VERSION 0001 // versi—n inicial
//#define VERSION 0002 // arreglos de interficie, ahora más independiente (08/02/2012)
//#define VERSION 0003 // Google por paises, verbatim, no funciona archive. (13/11/2012)
#define VERSION 0004 // Google por fechas sustituyendo archive. (13/11/2012)

class SearchEngine;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Ui::MainWindow *ui;

    void setEditConnection(SearchEngine* gs);
    bool SecurityFile();
    bool Login();

};

#endif // MAINWINDOW_H
