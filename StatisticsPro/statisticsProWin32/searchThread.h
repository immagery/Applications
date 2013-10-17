#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include "dataStructures.h"

#include <QtNetwork\qnetworkreply.h>
#include <QtNetwork\qnetworkproxy.h>
#include <QThread>

enum th_status { TH_NO_INIT = 0, TH_WAITING, TH_RUNNING, TH_FINISHED, TH_LOGIN};

class searchThread : public QThread
{
    Q_OBJECT

public:

    int idThread; // id del proceso.
    th_status status;

    bool useProxy;
    QString proxyDefinition;
    int proxyPort;
    QNetworkAccessManager networkManager;

    QString prefix01;
    QString prefix02;
    QString sufix01;
    QString sufix02;

    // datos
    requestData query;

    // verbatim
    bool useVerbatim;
	bool useDates;

    // Constructories
    searchThread();
    searchThread(int id);
    searchThread( const searchThread& st);
    void initData(requestData* data);
    void updateData(requestData* data);

    bool GetResultData(QString str, requestData& data);
    bool GetResultFromHtml(QString str, requestData& data);

    int doLoginSearch(QString PrevSearch);
    void doSearch();

    //QString doSearch(QString sQuery, int searchType, int id, QString sEdition);
    QString parseStringValue(QString replyUrl, QString sTag);
    //void nextQuery();

    void setProxy(QString sProxy);
    void disableProxy();

signals:
    void echoSearch();
    void echoLogin(int version);
    void esperarUnPoco();

public slots:
    void handleNetworkData(QNetworkReply *networkReply);





};

#endif // SEARCHTHREAD_H
