#include <QtWidgets\qapplication.h>
#include "mainwindow.h"

#define DEBUG_FLAG 1

#include "SearchEngine.h"
//#include "guicon.h"

int main(int argc, char *argv[])
{
	//RedirectIOToConsole();

    QApplication a(argc, argv);
    MainWindow w;

    SearchEngine engine;
    w.setEditConnection(&engine);

    // Verificaci—n de versi—n con el servidor en internet.
    // 0. hacer petici—n web
    int versionOficial = engine.getVersionId();

    if(versionOficial <= 0 )
        exit(0);

    //1.password -> no lo quieren activado
    //w.Login();

    // 2. Comparar versi—n.
    if(versionOficial > VERSION)
        exit(0);

    w.show();

    return a.exec();
}


