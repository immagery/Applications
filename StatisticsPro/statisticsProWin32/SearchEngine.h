#ifndef SearchEngine_H
#define SearchEngine_H

#include <QObject>
#include <QtWidgets\qlineedit.h>
#include <QtNetwork\qnetworkreply.h>
#include <QTimer>

#include "ui_mainwindow.h"
#include "dataStructures.h"
#include "searchThread.h"

class SearchEngine : public QObject
{
    Q_OBJECT

    public:
        SearchEngine();

        void setConnections();

        // configuración general
        QLineEdit *editor;
        Ui::MainWindow *ui;

        QVector<QString> proxies;
        QVector<QString> countries;
        bool bUseNewsCountries;
		bool bUseDates;
        bool bverbatimSeach;

        // Variables generales del proceso
        bool processLoaded; // Los datos de búsquedas están cargados
        bool processRunning; // El proceso está en marcha

        // datos de búsquedas.
        QVector<QVector<requestData> > queriesData; // datos de las búsquedas.
        searchThread queryLauncher;
        QVector<QStringList> dateRestrictions; // Restricciones de fechas para archive

        QStringList queryListNames;
        QStringList colListNames;

        int nQueryMatrixRows;
        int nQueryMatrixCols;

        int recoveredVersion;

        int currentQuery; // Indica en qué búsqueda estamos trabajando (valor absoluto)

        // Tiempos de espera.
        int interBlockWaitTime; // espera entre bloques
        int queryWaitTime; // espera entre búsquedas
        int currentIntervalWaitingTime; // Lo que se está esperando ahora.

        int queryBlockSize; // tamaño del bloque

        // Timers para hacer las esperas y contar el tiempo.
        QTimer queryWait;
        QTimer queryBlockWait;
        QTimer refreshInfoTriger;
        QTime timeCounter;

        bool useProxy;
        QString proxyDefinition;

        int lapsusLimit;

        // Numero de entornos de búsqueda (google, news, yahoo, archive...)
        bool searchEnvironments[NUM_OF_ENVIRONMENTS];

        QStringList archiveDates;

        // Lista donde guardaremos las cadenas de búsqueda con las que componer las búsquedas.
        QVector<QString> querySchemes;

        QVector<QString> prefix1, prefix2, sufix1, sufix2;

        // Carpeta de donde se ha hecho la última lectura.
        QString currentFolder;

    public slots:

        void recoverResult();

        bool readQueries();
        bool readDateRestrictions();
        bool readCountries(QString countriesFileName);
        void readProxies();

        void toogleEnvironments(bool);
        void processQueries();
        void doProcess();
        void esperaUnPoco();
        void selectFile();
        void clean();

        void allQueriesFinished();
        void toggleProcess();
        void setProcessButtons();

        void exportResults();

        void enableProxy(int);

        void setInEdit(QLineEdit *ed){editor = ed;}

        QString getEstado(th_status s1, th_status s2);
        QString getTipo(int s);
        QString getTipoShort(int s);
        void updateInfo();

        int getVersionId(bool debug = false);
        void catchVersionId(int id);

        int GetRow();
        int GetCol();

        int GetSearchDone();
        int GetSearchTodo();
        int GetTotalSearch();

};

class searchData
{
public:
    searchData()
    {
        sname = "";
        country = "";
        env = -1;

        ldate = ""; hdate = "";
        row = 0;
        col = 0;

        contrainByCountry = 0;
        useVerbatim = 0;
    }

    searchData(const searchData& envCopy)
    {
        sname = envCopy.sname;
        env = envCopy.env;
        ldate = envCopy.ldate;
        hdate = envCopy.hdate;
        row = envCopy.row;
        col = envCopy.col;
        country = envCopy.country;
        contrainByCountry = envCopy.contrainByCountry;
        useVerbatim = envCopy.useVerbatim;
    }

    QString sname;
    QString country;
    int env;

    QString ldate, hdate;
    int row;
    int col;

    bool contrainByCountry;
    bool useVerbatim;
	bool useDates;
};

#endif // SearchEngine_H
