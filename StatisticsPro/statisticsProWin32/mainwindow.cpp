#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "SearchEngine.h"

#include <QtWebKit\qwebelement.h>
#include <QtWebKitWidgets\qwebframe.h>
#include <QtWebKitWidgets\qwebview.h>

#include <QtWidgets\qinputdialog.h>
#include <QtWidgets\qtextedit.h>
#include <QtWidgets\qmessagebox.h>
#include <QFile>
#include <QTextStream>
#include <QDir>

#define PSW "tuercas"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

	//printf("Hay que volver a activar security file protection\n");
    if(!SecurityFile())
        exit(0);

    ui->setupUi(this);    
}

bool MainWindow::SecurityFile()
{
    QFile schemesFile(QDir::currentPath()+"/seqFile.dwg");
    if(schemesFile.exists())
        return true;

    return false;
}

bool MainWindow::Login()
{
    int intentos = 0;

    bool ok = true;
    while(intentos < 3 && ok)
    {
         QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                              tr("Password:"), QLineEdit::Password,
                                              "", &ok);
         if(text != PSW || text.isEmpty())
         {
             intentos++;
             QString name = "Aviso";
             QString text = "Password incorrecto, vuelva a probar. \nTienes %1 intentos mas.";
             QMessageBox(QMessageBox::Information,name, text.arg(3-intentos)).exec();
         }
         else
             return true;
     }

    return false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setEditConnection(SearchEngine* gs)
{
    gs->ui = ui;
    // Conexiones para google news
    connect(ui->process_button, SIGNAL(clicked()), gs, SLOT(toggleProcess()));
    connect(ui->selectFileButton, SIGNAL(clicked()), gs, SLOT(selectFile()));
    connect(ui->Clean_button, SIGNAL(clicked()), gs, SLOT(clean()));
    connect(ui->export_button, SIGNAL(clicked()), gs, SLOT(exportResults()));

    connect(ui->google_check, SIGNAL(toggled(bool)), ui->google_news_paises_check, SLOT(setEnabled(bool)));

    connect(ui->yahoo_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->yahoo_news_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_news_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_news_html_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_achive_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_news_paises_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
    connect(ui->google_search_vebatim_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));
	connect(ui->google_search_fechas_check, SIGNAL(toggled(bool)), gs, SLOT(toogleEnvironments(bool)));

    connect(ui->proxyCheck, SIGNAL(stateChanged(int)), gs, SLOT(enableProxy(int)));

    // Fijamos el tamaño de las celdas
    // Tabla de resultados de google news
    int idWidth001 = 50;
    ui->resultsTable->setColumnWidth(0,ui->resultsTable->width()-idWidth001);

    ui->Clean_button->setEnabled(false);
    ui->process_button->setEnabled(false);

    ui->block_size_edit->setText(QString("%1").arg(gs->queryBlockSize));
    ui->block_wait_time_edit->setText(QString("%1").arg(gs->interBlockWaitTime));
    ui->wait_time_edit->setText(QString("%1").arg(gs->queryWaitTime));

}
