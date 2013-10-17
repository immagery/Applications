#include "SearchEngine.h"

#include <QString>
#include <QtNetwork\qnetworkrequest.h>
#include <QtNetwork\qnetworkaccessmanager.h>
#include <QXmlStreamReader>
#include <QFile>
#include <QtWidgets\qmessagebox.h>
#include <QtWidgets\qfiledialog.h>
#include <QDir>
#include <QTextStream>
#include <QRegExp>
#include <QtNetwork\qnetworkproxy.h>

#include <QtWebkit\qwebelement.h>
#include <QtWebKitWidgets\qwebframe.h>
#include <QtWebKitWidgets\qwebview.h>

#include "queryReader.h"

using namespace std;

#ifdef _WIN32 // _WIN32 is defined by all Windows 32 and 64 bit compilers, but not by others.
#include <windows.h>
#endif

#include "searchThread.h"
#include <QTimer>

void readSchemes(QTextStream& in, QVector<QString>& querySchemes, QVector<QString>& prefix01, QVector<QString>& prefix02,
                 QVector<QString>& sufix01, QVector<QString>& sufix02)
{
    querySchemes.clear();
    querySchemes.resize(SCHEMES_COUNT);

    while(!in.atEnd())
    {
        QString front;
        in >> front;

        if(front == "GOOGLE_WEB")
        {
                querySchemes[WEB_SEARCH] = in.readLine();
                prefix01[WEB_SEARCH] = in.readLine();
                prefix02[WEB_SEARCH] = in.readLine();
                sufix01[WEB_SEARCH] = in.readLine();
                sufix02[WEB_SEARCH] = in.readLine();
        }
        else if( front == "GOOGLE_NEWS")
        {
                querySchemes[NEWS_SEARCH] = in.readLine();
                prefix01[NEWS_SEARCH]  = in.readLine();
                prefix02[NEWS_SEARCH] = in.readLine();
                sufix01[NEWS_SEARCH] = in.readLine();
                sufix02[NEWS_SEARCH] = in.readLine();
        }
        else if( front == "GOOGLE_NEWS_HTML")
        {
                querySchemes[NEWS_SEARCH_HTML] = in.readLine();
                prefix01[NEWS_SEARCH_HTML]  = in.readLine();
                prefix02[NEWS_SEARCH_HTML] = in.readLine();
                sufix01[NEWS_SEARCH_HTML] = in.readLine();
                sufix02[NEWS_SEARCH_HTML] = in.readLine();
        }
        else if(front == "YAHOO_WEB")
        {
                querySchemes[YAHOO_WEB_SEARCH] = in.readLine();
                prefix01[YAHOO_WEB_SEARCH]  = in.readLine();
                prefix02[YAHOO_WEB_SEARCH] = in.readLine();
                sufix01[YAHOO_WEB_SEARCH] = in.readLine();
                sufix02[YAHOO_WEB_SEARCH] = in.readLine();
        }
        else if(front == "YAHOO_NEWS")
        {
                querySchemes[YAHOO_NEWS_SEARCH] = in.readLine();
                 prefix01[YAHOO_NEWS_SEARCH]  = in.readLine();
                prefix02[YAHOO_NEWS_SEARCH]  = in.readLine();
                sufix01[YAHOO_NEWS_SEARCH] = in.readLine();
                sufix02[YAHOO_NEWS_SEARCH] = in.readLine();
        }
        else if(front == "ARCHIVE")
        {
                querySchemes[NEWS_ARCHIVE_SEARCH] = in.readLine();
                prefix01[NEWS_ARCHIVE_SEARCH] = in.readLine();
                prefix02[NEWS_ARCHIVE_SEARCH] = in.readLine();
                sufix01[NEWS_ARCHIVE_SEARCH] = in.readLine();
                sufix02[NEWS_ARCHIVE_SEARCH] = in.readLine();
        }
        else
        {
            in.readLine();
        }
    }
}

void writeSchemes(QTextStream& out, QVector<QString>& querySchemes, QVector<QString>& prefix01,
                  QVector<QString>& prefix02, QVector<QString>& sufix01, QVector<QString>& sufix02)
{
   out << QString("GOOGLE_WEB %1\n").arg(querySchemes[WEB_SEARCH]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[WEB_SEARCH]).arg(prefix02[WEB_SEARCH]).arg(sufix01[WEB_SEARCH]).arg(sufix02[WEB_SEARCH]);

   out << QString("GOOGLE_NEWS %1\n").arg(querySchemes[NEWS_SEARCH]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[NEWS_SEARCH]).arg(prefix02[NEWS_SEARCH]).arg(sufix01[NEWS_SEARCH]).arg(sufix02[NEWS_SEARCH]);

   out << QString("GOOGLE_NEWS_HTML %1\n").arg(querySchemes[NEWS_SEARCH_HTML]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[NEWS_SEARCH_HTML]).arg(prefix02[NEWS_SEARCH_HTML]).arg(sufix01[NEWS_SEARCH_HTML]).arg(sufix02[NEWS_SEARCH_HTML]);

   out << QString("YAHOO_WEB %1\n").arg(querySchemes[YAHOO_WEB_SEARCH]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[YAHOO_WEB_SEARCH]).arg(prefix02[YAHOO_WEB_SEARCH]).arg(sufix01[YAHOO_WEB_SEARCH]).arg(sufix02[YAHOO_WEB_SEARCH]);

   out << QString("YAHOO_NEWS %1\n").arg(querySchemes[YAHOO_NEWS_SEARCH]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[YAHOO_NEWS_SEARCH]).arg(prefix02[YAHOO_NEWS_SEARCH]).arg(sufix01[YAHOO_NEWS_SEARCH]).arg(sufix02[YAHOO_NEWS_SEARCH]);

   out << QString("ARCHIVE %1\n").arg(querySchemes[NEWS_ARCHIVE_SEARCH]);
   out << QString("%1\n%2\n%3\n%4\n").arg(prefix01[NEWS_ARCHIVE_SEARCH]).arg(prefix02[NEWS_ARCHIVE_SEARCH]).arg(sufix01[NEWS_ARCHIVE_SEARCH]).arg(sufix02[NEWS_ARCHIVE_SEARCH]);
}


SearchEngine::SearchEngine()
{
    // Inicializacion de veriables.
    //doneQueries = 0;
    readProxies();
    //readCountries();

    QStringList queryList;
    //searchDONE = 0;
    //searchTODO = 0;
    //totalSearchTODO = 0;

    bUseNewsCountries = false;
	bUseDates= false;

    nQueryMatrixCols = -1;
    nQueryMatrixRows = -1;
    currentQuery = -1;

    lapsusLimit = 0;

    processLoaded = false;
    processRunning = false;

    queryBlockSize = DEFAULT_QUERY_BLOCK;
    interBlockWaitTime = INTER_BlOCK_WAIT_TIME;
    queryWaitTime = WAIT_TIME;

    currentIntervalWaitingTime = 0;

    for(int i = 0; i< NUM_OF_ENVIRONMENTS; i++)
        searchEnvironments[i] = false;

    currentQuery = 0;
    queryWait.stop();
    queryBlockWait.stop();

    queryWait.setInterval(WAIT_TIME*1000); // Esta in milisegundos
    queryBlockWait.setInterval(INTER_BlOCK_WAIT_TIME*1000); // Esta in milisegundos.

    connect(&queryWait, SIGNAL(timeout()), this, SLOT(doProcess()));
    connect(&queryBlockWait, SIGNAL(timeout()), this, SLOT(doProcess()));

    refreshInfoTriger.stop();
    refreshInfoTriger.setInterval(500); // Cada medio segundo actualizamos la informacion.
    connect(&refreshInfoTriger, SIGNAL(timeout()), this, SLOT(updateInfo()));

    // connectamos el thread para poder recibir los signals.
    connect(&queryLauncher,SIGNAL(echoSearch()), this, SLOT(recoverResult()));
    connect(&queryLauncher,SIGNAL(esperarUnPoco()), this, SLOT(esperaUnPoco()));

    connect(&queryLauncher, SIGNAL(echoLogin(int)), this, SLOT(catchVersionId(int)));

    // Ponemos las cadenas por defecto.
    querySchemes.clear();
    querySchemes.resize(SCHEMES_COUNT);
    querySchemes[WEB_SEARCH] = GWEB_URL;
    querySchemes[NEWS_SEARCH] = GNEWS_URL;
    querySchemes[YAHOO_WEB_SEARCH] = YWEB_URL;
    querySchemes[YAHOO_NEWS_SEARCH] = YNEWS_URL;
    querySchemes[NEWS_ARCHIVE_SEARCH] = GNEWS_ARCHIVE_URL;
    //querySchemes[NEWS_SEARCH_HTML] = GNEWS_URL_HTML;
    querySchemes[NEWS_SEARCH_HTML] = GWEB_URL_VERBATIM;

    prefix1.clear();
    prefix1.resize(SCHEMES_COUNT);
    prefix1[WEB_SEARCH] = PREFIX01_GWEB;
    prefix1[NEWS_SEARCH] = PREFIX01_GNEWS;
    prefix1[YAHOO_WEB_SEARCH] = PREFIX01_YWEB;
    prefix1[YAHOO_NEWS_SEARCH] = PREFIX01_YNEWS;
    prefix1[NEWS_ARCHIVE_SEARCH] = PREFIX01_GNEWS_ARCH;
    prefix1[NEWS_SEARCH_HTML] = PREFIX01_GNEWS;

    prefix2.clear();
    prefix2.resize(SCHEMES_COUNT);
    prefix2[WEB_SEARCH] = PREFIX02_GWEB;
    prefix2[NEWS_SEARCH] = PREFIX02_GNEWS;
    prefix2[YAHOO_WEB_SEARCH] = PREFIX02_YWEB;
    prefix2[YAHOO_NEWS_SEARCH] = PREFIX02_YNEWS;
    prefix2[NEWS_ARCHIVE_SEARCH] = PREFIX02_GNEWS_ARCH;
    prefix2[NEWS_SEARCH_HTML] = PREFIX02_GNEWS;

    sufix1.clear();
    sufix1.resize(SCHEMES_COUNT);
    sufix1[WEB_SEARCH] = SUFIX01_GWEB;
    sufix1[NEWS_SEARCH] = SUFIX01_GNEWS;
    sufix1[YAHOO_WEB_SEARCH] = SUFIX01_YWEB;
    sufix1[YAHOO_NEWS_SEARCH] = SUFIX01_YNEWS;
    sufix1[NEWS_ARCHIVE_SEARCH] = SUFIX01_GNEWS_ARCH;
    sufix1[NEWS_SEARCH_HTML] = SUFIX01_GNEWS;

    // Existe pero no se usa.
    sufix2.clear();
    sufix2.resize(SCHEMES_COUNT);
    sufix2[WEB_SEARCH] = SUFIX02_GWEB;
    sufix2[NEWS_SEARCH] = SUFIX02_GWEB;
    sufix2[YAHOO_WEB_SEARCH] = SUFIX02_GWEB;
    sufix2[YAHOO_NEWS_SEARCH] = SUFIX02_GWEB;
    sufix2[NEWS_ARCHIVE_SEARCH] = SUFIX02_GWEB;
    sufix2[NEWS_SEARCH_HTML] = SUFIX02_GWEB;

    // Leemos par�metros de configuraci�n.
    // 0. Cadenas de b�squeda
    QFile schemesFile(QDir::currentPath()+"/searchStringSchemes.txt");
    if(schemesFile.exists())
    {
        schemesFile.open(QFile::ReadOnly);
        QTextStream in(&schemesFile);
        readSchemes(in, querySchemes, prefix1, prefix2, sufix1, sufix2);
    }
    else
    {
        schemesFile.open(QFile::WriteOnly);
        QTextStream out(&schemesFile);
        writeSchemes(out, querySchemes, prefix1, prefix2, sufix1, sufix2);
    }

    // 1. Directorio donde se leen los ficheros de b�squeda.
    QFile currentFolderFile(QDir::currentPath()+"/currentPath.txt");
    if(currentFolderFile.exists())
    {
        currentFolderFile.open(QFile::ReadOnly);
        QTextStream in(&currentFolderFile);
        currentFolder = in.readLine();
    }
    else
    {
        currentFolderFile.open(QFile::WriteOnly);
        QTextStream out(&currentFolderFile);
        // Ponemos el lugar de trabajo por defecto.
        currentFolder = QDir::currentPath();
        out << currentFolder;
    }

    queryLauncher.start();
}

int SearchEngine::GetRow()
{
    if(currentQuery >= 0 && nQueryMatrixCols >= 0)
	{
		if(nQueryMatrixCols == 0)
			return -1;

        return currentQuery/nQueryMatrixCols;
	}
    return -1;
}

int SearchEngine::GetCol()
{
    if(currentQuery >= 0 && nQueryMatrixCols >= 0)
	{
		if(nQueryMatrixCols == 0)
			return -1;

        return currentQuery%nQueryMatrixCols;
	}

    return -1;
}

int SearchEngine::GetSearchDone()
{
    return currentQuery;
}

int SearchEngine::GetSearchTodo()
{
    return (nQueryMatrixCols*nQueryMatrixRows)-currentQuery;
}

int SearchEngine::GetTotalSearch()
{
    return nQueryMatrixCols*nQueryMatrixRows;
}

void SearchEngine::clean()
{

    ui->resultsTable->clear();

    queryListNames.clear();
    archiveDates.clear();
    dateRestrictions.clear();

    ui->selectFileButton->setEnabled(true);
    ui->process_button->setEnabled(false);
    ui->process_button->setText("Procesar");
    ui->Clean_button->setEnabled(false);

    ui->google_check->setEnabled(false);
    ui->google_search_vebatim_check->setEnabled(false);
	ui->google_search_fechas_check->setEnabled(false);
    ui->yahoo_check->setEnabled(false);
    ui->yahoo_news_check->setEnabled(false);
    ui->google_achive_check->setEnabled(false);
    ui->google_news_check->setEnabled(false);

    processLoaded = false;
    processRunning = false;

    // Reiniciamos todos los valores.
    SearchEngine();

}

void SearchEngine::enableProxy(int state)
{
    state = 0;
    if(ui->proxyCheck->checkState() == Qt::Checked)
    {
        ui->proxyEdit->setEnabled(true);
    }
    else
    {
        ui->proxyEdit->setEnabled(false);
    }
}

/*
QString getProxyHost()
{
    QString proxyName(ui->proxyEdit);
    QString proxyPort(proxyName.right(proxyName.size()-proxyName.indexOf(":")-1));
    proxyName = proxyName.left(proxyName.size()-proxyPort.size()-1);
    return proxyName;
}

int getProxyPort()
{
    QString proxyName(ui->proxy);
    QString proxyPort(proxyName.right(proxyName.size()-proxyName.indexOf(":")-1));
    return proxyPort.toInt();
}
*/

void SearchEngine::setConnections()
{
}

void SearchEngine::esperaUnPoco()
{
    //queryWait.stop();
    //queryWait.setInterval(10*60*1000); // esperara 10 minutos.
    //queryWait.start();
}

void SearchEngine::doProcess()
{
    queryWait.stop();
    queryBlockWait.stop();

    // Volvemos a asignar el tiempo normal del trigger.
    queryWait.setInterval(queryWaitTime*1000);
    queryBlockWait.setInterval(interBlockWaitTime*1000);

    // Asignamos flags de comportamiento
    queryLauncher.useVerbatim = bverbatimSeach;

    // Si esta libre el procesador lanzamos nuevas busquedas
    if(currentQuery < GetTotalSearch()-1)
    {
        if(queryLauncher.status == TH_FINISHED || queryLauncher.status == TH_NO_INIT)
        {
            // Todavia quedan busquedas por hacer.
            if(queryLauncher.status != TH_NO_INIT)
                currentQuery++;
            queryLauncher.initData(&queriesData[GetRow()][GetCol()]);

            queryLauncher.prefix01 = prefix1[queriesData[GetRow()][GetCol()].requestType];
            queryLauncher.prefix02 = prefix2[queriesData[GetRow()][GetCol()].requestType];
            queryLauncher.sufix01 = sufix1[queriesData[GetRow()][GetCol()].requestType];
            queryLauncher.sufix02 = sufix2[queriesData[GetRow()][GetCol()].requestType];


            if(useProxy)
                queryLauncher.setProxy(ui->proxyEdit->text());

            queryLauncher.doSearch();
        }
        else
        {
            requestData& current = queriesData[GetRow()][GetCol()];
            current.waitLapsusCounter++; // Indicamos que esto se esta poniendo feo

            if(current.waitLapsusCounter > lapsusLimit && lapsusLimit > 0)
            {
                // Lanzamos mensaje de aviso.
            }
        }
    }
    else
    {
        //Se han acabado las busquedas.
        allQueriesFinished();
        processRunning = false;
        return;
    }

    processRunning = true;

    // Ponemos en marcha el contador para la siguente busqueda.
    if(queryBlockSize > 0)
    {
        if(currentQuery%queryBlockSize == 0 && currentQuery!= 0)
        {
            currentIntervalWaitingTime = queryBlockWait.interval();
            queryBlockWait.start();
            timeCounter.start();
            return;
        }
        else
        {
            currentIntervalWaitingTime = queryWait.interval();
            queryWait.start();
            timeCounter.start();
            return;
        }
    }
    else
    {
        currentIntervalWaitingTime = queryWait.interval();
        queryWait.start();
        timeCounter.start();
        return;
    }
}

void SearchEngine::toggleProcess()
{
    if(processRunning)
    {
        queryWait.stop();
        queryBlockWait.stop();
        refreshInfoTriger.stop();

        ui->infoBar->setText("> El usuario a interrumpido las busquedas...");

        ui->process_button->setEnabled(true);

        ui->block_size_edit->setEnabled(true);
        ui->wait_time_edit->setEnabled(true);
        ui->block_wait_time_edit->setEnabled(true);

        ui->selectFileButton->setEnabled(true);
        ui->export_button->setEnabled(true);
        ui->Clean_button->setEnabled(true);

        ui->process_button->setText("Reanudar");

        updateInfo();
        processRunning = false;
    }
    else
    {
        if(!processLoaded)
        {
            processQueries();
        }
        else
        {
            setProcessButtons();
            // Para que arranque la primera vez.
            doProcess();
        }
    }
}

void SearchEngine::allQueriesFinished()
{
    ui->process_button->setEnabled(true);

    ui->block_size_edit->setEnabled(true);
    ui->wait_time_edit->setEnabled(true);
    ui->block_wait_time_edit->setEnabled(true);

    ui->selectFileButton->setEnabled(true);
    ui->export_button->setEnabled(true);
    ui->Clean_button->setEnabled(true);

    updateInfo();
    refreshInfoTriger.stop();

    QString name = "Aviso";
    QString text = "El proceso de busquedas se ha acabado.";
    QMessageBox(QMessageBox::Information,name, text).exec();
    return;

}

QString SearchEngine::getEstado(th_status s1, th_status s2)
{
    QString estado;
    if(s1 == TH_WAITING)
    {
        estado = "esperando";
    }
    else if(s2 == TH_FINISHED)
    {
        estado = "acabado";
    }
    else
    {
        estado = "procesando";
    }
    return estado;
}

QString SearchEngine::getTipo(int s)
{
    QString estado;
    if(s == YAHOO_WEB_SEARCH)
    {
        estado = "Yahoo Web";
    }
    else if(s == NEWS_SEARCH )
    {
        estado = "Google News";
    }
    else if(s == NEWS_SEARCH_HTML )
    {
        estado = "Google News Html";
    }
    else if(s == NEWS_ARCHIVE_SEARCH)
    {
        estado = "Google Archive";
    }
    else if(s == YAHOO_NEWS_SEARCH)
    {
        estado = "Yahoo news";
    }
    else if(s == WEB_SEARCH)
    {
        estado = "Google Web";
    }
    else
        estado = "nose";

    return estado;
}

QString SearchEngine::getTipoShort(int s)
{
    QString estado;
    if(s == YAHOO_WEB_SEARCH)
    {
        estado = "YWeb";
    }
    else if(s == YAHOO_NEWS_SEARCH)
    {
        estado = "YNews";
    }
    else if(s == WEB_SEARCH)
    {
        estado = "GWeb";
    }
    else if(s == NEWS_SEARCH )
    {
        estado = "GNews";
    }
    else if(s == NEWS_SEARCH_HTML )
    {
        estado = "GNewsHtml";
    }
    else if(s == NEWS_ARCHIVE_SEARCH)
    {
        estado = "Archive";
    }
    else
        estado = "nose";

    return estado;
}

void SearchEngine::updateInfo()
{
    QString infoProc("Busqueda: %1\nTipo:%2\nEstado:%3\n\n Siguente en\n %4 segundos\n");

    if(currentQuery >= 0)
    {
        requestData& current = queriesData[GetRow()][GetCol()];

        // Pintamos el estado de los procesos cargados
        int restingTime = currentIntervalWaitingTime - timeCounter.elapsed();
        if(restingTime < 0 )
            restingTime = -1;

        QString secs = QString("%1").arg((float)restingTime/1000.0, 4, 'f',1);

        ui->infoProceso->setText(infoProc.arg(current.requestId).arg(getTipo(current.requestType)).arg(getEstado(queryLauncher.status,queryLauncher.status)).arg(secs));
        ui->infoBar->setText(QString("> %1").arg(queryListNames[GetRow()]));

        if(currentIntervalWaitingTime > 0)
           ui->waitingTimeBar->setValue(((float)timeCounter.elapsed()/(float)currentIntervalWaitingTime)*100);
    }
}

/*
void SearchEngine::recoverLostQueries()
{
    // Guardamos el vector de peticiones que tenemos que rehacer y liberamos el vector
    // para las peticiones que podrían salir mal.
    QVector<int> queriesForSolve;
    queriesForSolve.resize(nonSolvedRequests.size());
    for(int i = 0; i< queriesForSolve.size(); i++)
        queriesForSolve[i] = nonSolvedRequests[i];
    nonSolvedRequests.clear();

    // realizamos las nuevas peticiones
    int i = doneQueries;
    for(; i < queriesForSolve.size() && runningQueries < maxParalelQueries; i++)
    {
        int slot = -1;
        for(int k = 0; k< maxParalelQueries; k++)
            if(processData.slotStore[k].free)
                slot = k;

        if(slot < 0)
                break;

        QString ed, hl;

        if(ui->ed_check->isChecked())
            ed = "us";

        if(ui->lan_check->isChecked())
            hl = "en";

        int newId = queriesForSolve[i];

        QString newUrl = doSearch(queryList[newId], NEWS_SEARCH, newId, hl, ed);

        processData.slotStore[slot].free = false;
        processData.slotStore[slot].talbeID = newId;
        processData.slotStore[slot].timer.start();
        runningQueries++;
        doneQueries = i;

        // Ahora esperaremos un poco más.
        #ifdef _WIN32 // _WIN32 is defined by all Windows 32 and 64 bit compilers, but not by others.
        Sleep(10000);
        #else
        sleep(10);
        #endif
    }

    doneQueries = i;
}
*/

// Exporta los resultados a un fichero cvs para importar en otros programas
void SearchEngine::exportResults()
{
    QFileDialog outFileDialog(0, "Save as...", QDir::currentPath());
    outFileDialog.setFileMode(QFileDialog::AnyFile);
    outFileDialog.setAcceptMode(QFileDialog::AcceptSave);

    QStringList fileNames;
     if (outFileDialog.exec())
         fileNames = outFileDialog.selectedFiles();

     if(fileNames.size() > 0)
     {
        QFile outFile(fileNames[0] + ".csv");
        outFile.open(QFile::WriteOnly);

        QTextStream out(&outFile);

        //out << "busquedas,"; // tenemos que dejar un espacio para la columna de busquedas.
        if(colListNames.size()>0)
        {
            for(int i = 0; i< colListNames.size(); i++)
            {
                QString ss = colListNames[i];
                out << ss;
                out << ",";
            }
            out << endl;
        }

        for(int i = 0; i< queriesData.size() ; i++)
        {
            out << queryListNames[i];
            for(int j = 0; j< queriesData[i].size() ; j++)
            {
                out << ",";
                out << queriesData[i][j].estimatedResultCount;
            }

            out << endl;
        }
        outFile.close();

        QString name = "Save as...";
        QString text = "Los datos se han exportado sin problemas.";
        QMessageBox(QMessageBox::Information,name, text).exec();
    }
}

// Lee los paises en los que se quiere hacer alguna búsqueda
bool SearchEngine::readCountries(QString countriesFileName)
{
    QFile inFile(QDir::currentPath()+"/"+countriesFileName);
    if(!inFile.exists())
        return false;

    inFile.open(QFile::ReadOnly);
    QTextStream in(&inFile);

    //int queryCounter = 0;
    while(!in.atEnd())
    {
        // Leemos linea a linea el contenido y lo parseamos para que se pueda buscar directamente.
        QString query = in.readLine().trimmed();

        // En el caso de que haya una linea en blanco la saltamos.
        if(query.isEmpty())
            continue;

        countries.push_back(query);
    }

    return true;
}

// Lee los proxies con los que se quiere hacer alguna búsqueda
void SearchEngine::readProxies()
{
    QString proxiesFileName = "proxies.txt";
    QFile inFile(QDir::currentPath()+"/"+proxiesFileName);
    if(!inFile.exists())
    {
        QString name = "Aviso";
        QString text = "El fichero de proxies no está creado";
        QMessageBox(QMessageBox::Information,name, text);
        return;
    }

    inFile.open(QFile::ReadOnly);
    QTextStream in(&inFile);

    //int queryCounter = 0;
    while(!in.atEnd())
    {
        // Leemos línea a línea el contenido y lo parseamos para que se pueda buscar directamente.
        QString query = in.readLine().trimmed();

        // En el caso de que haya una línea en blanco la saltamos.
        if(query.isEmpty())
            continue;

        proxies.push_back(query);
    }
}

// Selecciona un fichero
void SearchEngine::selectFile()
{   
    QFileDialog inFileDialog(0, "Selecciona un fichero", currentFolder);
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
     if (inFileDialog.exec())
         fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    currentFolder = inFileDialog.directory().path();

    // Guardamos el path
    QFile currentFolderFile(QDir::currentPath()+"/currentPath.txt");
    currentFolderFile.open(QFile::WriteOnly);
    QTextStream out(&currentFolderFile);
    out << currentFolder;

    ui->lineEdit->setText(fileNames[0]);

    bool success = readQueries();

    if(success)
    {
        ui->Clean_button->setEnabled(true);
        ui->process_button->setEnabled(true);

        ui->google_check->setEnabled(true);
        ui->google_news_paises_check->setEnabled(true);
        ui->google_search_vebatim_check->setEnabled(true);
		ui->google_search_fechas_check->setEnabled(true);
        ui->yahoo_check->setEnabled(true);
        ui->yahoo_news_check->setEnabled(true);
        ui->google_achive_check->setEnabled(true);
        ui->google_news_check->setEnabled(true);
        ui->google_news_html_check->setEnabled(true);

        ui->google_check->setChecked(true);
        ui->google_search_vebatim_check->setChecked(false);
		ui->google_search_fechas_check->setChecked(false);
        searchEnvironments[WEB_SEARCH] = true;
        ui->yahoo_check->setChecked(false);
        ui->yahoo_news_check->setChecked(false);
        ui->google_achive_check->setChecked(false);
        ui->google_news_check->setChecked(false);
        ui->google_news_html_check->setChecked(false);
    }
}

// Lee las búsquedas que se quiere hacer
bool SearchEngine::readQueries()
{
    if(!QFile(ui->lineEdit->text()).exists())
    {
        QString name = "Aviso";
        QString text = "El fichero especificado no existe";
        QMessageBox(QMessageBox::Information,name, text);
        return false;
    }

    // Leemos las busquedas
    QString sLine = ui->lineEdit->text();
    int numQueries = 0;

    // Solo leemos las búsquedas.
    numQueries = readFile(queryListNames, sLine);
    return true;
}

bool SearchEngine::readDateRestrictions()
{
    if(!QFile(ui->lineEdit->text()).exists())
    {
        QString name = "Aviso";
        QString text = "El fichero especificado no existe";
        QMessageBox(QMessageBox::Information,name, text);
        return false;
    }

    // Leemos las busquedass
    QString sLine = ui->lineEdit->text();
    int numDateRestrictions = -1;

    // Solo leemos las búsquedas.
    numDateRestrictions = readDatesFile(dateRestrictions, sLine);

    ui->infoBar->setText(QString("Cargando el fichero %1 con %2 restricciones").arg(sLine).arg(numDateRestrictions));

    if(numDateRestrictions < 0)
        return false;

    return true;
}

void SearchEngine::toogleEnvironments(bool)
{
    searchEnvironments[YAHOO_WEB_SEARCH] = ui->yahoo_check->isChecked();
    searchEnvironments[YAHOO_NEWS_SEARCH] = ui->yahoo_news_check->isChecked();
    searchEnvironments[NEWS_SEARCH] = ui->google_news_check->isChecked();

    searchEnvironments[NEWS_SEARCH_HTML] = ui->google_news_html_check->isChecked();

    searchEnvironments[WEB_SEARCH] = ui->google_check->isChecked();

	if(searchEnvironments[WEB_SEARCH])
	{
		if(ui->google_search_fechas_check->isChecked())
		{
			if(!readDateRestrictions())
			{
				ui->google_search_fechas_check->setChecked(false);
				QMessageBox(QMessageBox::Information,"Aviso", "El fichero de restriccion de fechas no existe.").exec();
				bUseDates = false;
			}
			else
			{
				bUseDates = true;
			}
		}
		else
		{
			bUseDates = false;
		}
	}

    if(!bUseNewsCountries && ui->google_news_paises_check->isChecked())
    {
        if(readCountries("countries_constraint.txt"))
            bUseNewsCountries = ui->google_news_paises_check->isChecked();
        else
        {
            ui->google_news_paises_check->setChecked(false);
            QMessageBox(QMessageBox::Information,"Aviso", "El fichero de paises no existe.").exec();
        }
    }
    else
    {
        bUseNewsCountries = false;
        countries.clear();
    }

    if(!searchEnvironments[NEWS_ARCHIVE_SEARCH] && ui->google_achive_check->isChecked())
    {
        if(readDateRestrictions())
            searchEnvironments[NEWS_ARCHIVE_SEARCH] = ui->google_achive_check->isChecked();
        else
        {
            ui->google_achive_check->setChecked(false);
            QMessageBox(QMessageBox::Information,"Aviso", "El fichero de restriccion de fechas no existe.").exec();
        }
    }
    else
        searchEnvironments[NEWS_ARCHIVE_SEARCH] = ui->google_achive_check->isChecked();

    bverbatimSeach = ui->google_search_vebatim_check->isChecked();
	bUseDates = ui->google_search_fechas_check->isChecked();
    queryLauncher.useVerbatim = bverbatimSeach;

    bool someEnv = false;
    for(int i = 0; i< NUM_OF_ENVIRONMENTS; i++)
       someEnv |= searchEnvironments[i];

    if(!someEnv)
        ui->process_button->setEnabled(false);
    else
        ui->process_button->setEnabled(true);

}

void SearchEngine::processQueries()
{
    // Iniciamos la lista de nombres de columnas.
    colListNames.clear();
    colListNames.push_front(QString("Busquedas"));

    // calculamos el numero total de busquedas que se realizaran.
    int numEnv = 0;
    int normalEnv = 0;
    int archive_env = 0;

    // Nos dice en que columna esta cada elemento.
    QVector< searchData > searchEnvironmentsData;

    QVector <int> traduction;
    traduction.fill(-1,NUM_OF_ENVIRONMENTS + dateRestrictions.size());

    for(int env = 0; env<NUM_OF_ENVIRONMENTS; env++)
    {
        if(searchEnvironments[env])
        {
            searchData d;
            switch(env)
            {
            case NEWS_ARCHIVE_SEARCH:
                //totalNumOfSearches += queryList.size()*dateRestrictions.size();
                for(int i = 0; i< dateRestrictions.size(); i++)
                {
                    d.sname = QString("[%1]\n[%2]").arg(dateRestrictions[i][0]).arg(dateRestrictions[i][1]);
                    d.ldate = dateRestrictions[i][0];
                    d.hdate = dateRestrictions[i][1];
                    d.col = searchEnvironmentsData.size();
                    d.env = env;
                    searchEnvironmentsData.push_back(d);
					d.useDates = true;

                    //colListNames.push_back(QString("[%1]\n[%2]").arg(dateRestrictions[i][0]).arg(dateRestrictions[i][1]));
                    //traduction[env+i] = normalEnv+archive_env;
                    //archive_env++;
                    //numEnv++;
                }
                break;

            case WEB_SEARCH:
                if(bUseNewsCountries && countries.size() > 0)
                {
                    for(int i = 0; i< countries.size(); i++)
                    {
                        d.sname = QString("Google\n[%1]").arg(countries[i]);
                        d.env = env;
                        d.col = searchEnvironmentsData.size();
                        d.country = countries[i];
						d.useDates = false;
                        d.contrainByCountry = true;
                        searchEnvironmentsData.push_back(d);
                    }
                }
				else if(bUseDates && dateRestrictions.size() > 0)
				{
					for(int i = 0; i< dateRestrictions.size(); i++)
                    {
                        d.sname = QString("Google [%1]\n[%2]").arg(dateRestrictions[i][0]).arg(dateRestrictions[i][1]);
                        d.env = env;
						d.ldate = dateRestrictions[i][0];
						d.hdate = dateRestrictions[i][1];
                        d.col = searchEnvironmentsData.size();
                        searchEnvironmentsData.push_back(d);
						d.useDates = true;
						d.contrainByCountry = false;
                    }
				}
				else
                {
                    d.sname = getTipoShort(env);
                    d.env = env;
                    d.col = searchEnvironmentsData.size();
                    d.contrainByCountry = false;
					d.useDates = false;
                    searchEnvironmentsData.push_back(d);
                }

                //colListNames.push_back(getTipoShort(env));
                //traduction[env] = normalEnv;
                //normalEnv++;
                //numEnv++;
                break;

            default:

                d.sname = getTipoShort(env);
                d.env = env;
                d.col = searchEnvironmentsData.size();
                searchEnvironmentsData.push_back(d);

                //totalNumOfSearches += queryList.size();
                //colListNames.push_back(getTipoShort(env));
                //traduction[env] = normalEnv;
                //normalEnv++;
                //numEnv++;
                break;
            }
        }
     }

    int numCols = searchEnvironmentsData.size();

    // Dimensionamos la matriz para los datos.
    queriesData.resize(queryListNames.size());
    for(int i = 0; i < queriesData.size(); i++)
        //queriesData[i].resize(numEnv);
        queriesData[i].resize(numCols);

    nQueryMatrixCols = numCols;
    nQueryMatrixRows = queryListNames.size();


    int element = 0;

    // inicializamos los threads para funcionar
    for(int search = 0; search < queriesData.size(); search++)
    {
        for(int envId = 0; envId < searchEnvironmentsData.size(); envId++)
        {
            int env = searchEnvironmentsData[envId].env;

            // en el caso de busquedas en archive usaremos las fechas.      
            int row = search;
            int col = searchEnvironmentsData[envId].col;
            queriesData[row][col].setData(queryListNames[search], element, env);
            queriesData[row][col].row = row;
            queriesData[row][col].col = col;

			if(env == NEWS_ARCHIVE_SEARCH || env == WEB_SEARCH)
			{
				queriesData[row][col].useDateConstraint = (env == NEWS_ARCHIVE_SEARCH);

				if(bUseDates)
					queriesData[row][col].useDateConstraint = true;

				queriesData[row][col].ldate = searchEnvironmentsData[envId].ldate;
				queriesData[row][col].hdate = searchEnvironmentsData[envId].hdate;
			}

            queriesData[row][col].country = searchEnvironmentsData[envId].country;
            queriesData[row][col].useCountryConstraint = searchEnvironmentsData[envId].contrainByCountry;


            element++;
        }
    }

    // Cargamos los resultados en las listas
    int resWidth001 = 100;
    int idWidth001 = 50;
    int numOFColumns = searchEnvironmentsData.size()+1;
    int minSearchNamesColWidth = 200;

    // Fijamos los tamanos en la tabla de la interficie
    ui->resultsTable->setRowCount(queriesData.size());
    ui->resultsTable->setColumnCount(numOFColumns);

    for(int i = 0; i < numOFColumns; i++)
        ui->resultsTable->setColumnWidth(i+1,resWidth001);

    if(ui->resultsTable->width()-resWidth001*numOFColumns-idWidth001 >  minSearchNamesColWidth)
        minSearchNamesColWidth = ui->resultsTable->width()-resWidth001*numOFColumns-idWidth001;

    ui->resultsTable->setColumnWidth(0,minSearchNamesColWidth);

    //colListNames.clear();
    for(int i = 0; i< searchEnvironmentsData.size(); i++)
        colListNames.push_back(searchEnvironmentsData[i].sname);

    ui->resultsTable->setHorizontalHeaderLabels(colListNames);

    //Rellenamos la tabla
    for(int i = 0; i< queryListNames.size(); i++)
    {
        QTableWidgetItem *rowItem = new QTableWidgetItem(queryListNames[i]);
        ui->resultsTable->setItem(i,0,rowItem);
    }

    setProcessButtons();
    processLoaded = true;
    doProcess();

}

void SearchEngine::setProcessButtons()
{
    QString sTemp = ui->block_size_edit->text();
    if(!sTemp.isEmpty() && sTemp.toInt() > 0)
        queryBlockSize = ui->block_size_edit->text().toInt();
    else
        queryBlockSize = DEFAULT_QUERY_BLOCK;

    sTemp = ui->block_wait_time_edit->text();
    if(!sTemp.isEmpty() && sTemp.toInt() >= 0 && sTemp.toInt() < 99999 )
        interBlockWaitTime = ui->block_wait_time_edit->text().toInt();
    else
        interBlockWaitTime = INTER_BlOCK_WAIT_TIME;

    sTemp = ui->wait_time_edit->text();
    if(!sTemp.isEmpty() && sTemp.toInt() >= 0 && sTemp.toInt() < 99999 )
        queryWaitTime = ui->wait_time_edit->text().toInt();
    else
        queryWaitTime = WAIT_TIME;

    ui->block_size_edit->setEnabled(false);
    ui->block_wait_time_edit->setEnabled(false);
    ui->wait_time_edit->setEnabled(false);

    ui->selectFileButton->setEnabled(false);
    ui->export_button->setEnabled(false);
    ui->Clean_button->setEnabled(false);

     refreshInfoTriger.start();

     // Deshabilitamos el boton de procesado porque ya est� creado.
     ui->process_button->setText("Parar");
}

int SearchEngine::getVersionId(bool debug)
{
    if(debug)
        return true;

    return queryLauncher.doLoginSearch(QString(LOGIN_URL));
}

void SearchEngine::catchVersionId(int id)
{
    recoveredVersion = id;
}

void SearchEngine::recoverResult()
{
    QColor c(210, 50, 50);
    QColor c2(50, 50, 50);
    QTableWidgetItem *value;
    QTableWidgetItem* it;

    requestData& current = queriesData[GetRow()][GetCol()];
    queryLauncher.updateData(&current);

    int row = current.row;
    int col = current.col;

    // Control de errores.
    switch(current.status)
    {
        case 403: // Notificacion de abuso
            value = new QTableWidgetItem(QString("BLOQUED"));
            it = ui->resultsTable->item(row, 1);
            if(it) it->background().setColor(c);
        break;
        case CANT_RECOVERY_DATA:
            value = new QTableWidgetItem(QString("NO DATA"));
            it = ui->resultsTable->item(row, 1);
            if(it) it->background().setColor(c2);
        break;
        default:
            value = new QTableWidgetItem(QString("%1").arg(current.estimatedResultCount));
        break;
    }

    ui->resultsTable->setItem(row,col+1,value);

    if(GetTotalSearch() != 0)
    {
        ui->progressBar->setValue((int)(((float)GetSearchDone()/(float)GetTotalSearch())*100.0));
        ui->processPercentageLabel->setText(QString("%1%").arg((int)(((float)GetSearchDone()/(float)GetTotalSearch())*100.0)));
    }

    updateInfo();
}
