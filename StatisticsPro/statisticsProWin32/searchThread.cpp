#include "searchThread.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QtWebKitWidgets\qwebframe.h>
#include <QtWebKitWidgets\qwebview.h>
#include <QtWebKit\qwebelement.h>
#include <QEventLoop>
#include <QUrlQuery>
#include <QUrl>

#include <iostream>
#include <fstream>

#define DEBUG_SEARCHES true

searchThread::searchThread(int id)
{
    idThread = id;
    useProxy = false;
    proxyDefinition = "";
    proxyPort = 0;

    if(useProxy)
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(default_proxy_name);
        proxy.setPort(default_proxy_port);
        networkManager.setProxy(proxy);
    }

    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(handleNetworkData(QNetworkReply*)), Qt::AutoConnection);

}

searchThread::searchThread()
{
    idThread = 0;
    useProxy = 0;
    useVerbatim = 0;
    proxyDefinition = "";
    proxyPort = 0;
    status = TH_NO_INIT;

    prefix01 = "";
    prefix02 = "";
    sufix01 = "";
    sufix02= "";

    if(useProxy)
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(default_proxy_name);
        proxy.setPort(default_proxy_port);
        networkManager.setProxy(proxy);
    }

    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
                 this, SLOT(handleNetworkData(QNetworkReply*)), Qt::AutoConnection);

}


searchThread::searchThread( const searchThread& st):QThread()
{
    idThread = st.idThread;
    status = st.status;
    useVerbatim = st.useVerbatim;

    prefix01 = st.prefix01;
    prefix02 = st.prefix02;
    sufix01 = st.sufix01;
    sufix02= st.sufix02;

    if(st.useProxy)
    {
        QNetworkProxy proxy;
        useProxy = true;
        proxyDefinition = st.proxyDefinition;
        proxyPort = st.proxyPort;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(proxyDefinition);
        proxy.setPort(proxyPort);
        networkManager.setProxy(proxy);
    }

    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
                 this, SLOT(handleNetworkData(QNetworkReply*)),Qt::AutoConnection);
}

void searchThread::setProxy(QString sProxy)
{
    if(sProxy.isEmpty())
        return;

    // Parsing proxy data
    QString proxyName(sProxy);
    QString sProxyPort(proxyName.right(proxyName.size()-proxyName.indexOf(":")-1));
    proxyName = proxyName.left(proxyName.size()-sProxyPort.size()-1);

    useProxy = true;
    proxyDefinition = proxyName;
    proxyPort = sProxyPort.toInt();

    // Setting proxy port
    QNetworkProxy nwproxy;
    nwproxy.setType(QNetworkProxy::HttpProxy);
    nwproxy.setHostName(proxyName);
    nwproxy.setPort(proxyPort);
    networkManager.setProxy(nwproxy);
}

void searchThread::disableProxy()
{
    useProxy = false;
    QNetworkProxy nwproxy;
    nwproxy.setType(QNetworkProxy::HttpProxy);
    networkManager.setProxy(nwproxy);
}

void searchThread::initData(requestData* data)
{
     query = *data;
     status = TH_WAITING;
}

void searchThread::updateData(requestData* data)
{
    int tmp = data->waitLapsusCounter;
    *data = query;
    data->waitLapsusCounter = tmp;
}


// Devuelve el valor correspondiente a ese parametro.
QString searchThread::parseStringValue(QString replyUrl, QString sTag)
{
    int stringLength = sTag.length();
    int valueBegin = replyUrl.indexOf(sTag);
    int valueEnd = replyUrl.indexOf("&", valueBegin+1);

    QString value = replyUrl.right(replyUrl.length()-(valueBegin+stringLength+1)).left(valueEnd-(valueBegin+stringLength)-1);
    return value;
}

int sign(int value)
{
	return (int)((float)value/fabs((float)value));
}

int truncateValue(float value)
{
	return (int)floor(value);
}

int roundDate(float value)
{
	return (int)ceil(value);
}

int getJulianDate(int day, int month, int year)
{
	int K = year;
	int M = month;
	int I = day;
	int UT = 12;

	int JD = 367*K - truncateValue((7*(K+truncateValue((M+9)/12)))/4) + 
					 truncateValue((275*M)/9) + I + 1721013.5 + UT/24 - 
					 0.5*sign(100*K+M-190002.5) + 0.5;

	return JD;

}


void searchThread::handleNetworkData(QNetworkReply *networkReply)
{
    //printf("\n\ndatos_recibidos\n\n");
    fflush(0);

    int errorNum = 0;
    if (!networkReply->error())
    {
        QString replyUrl = networkReply->request().url().toString();

        if(replyUrl.contains("ldConstantinopolis")) // en este caso era una petici—n de autenticaci—n
            return;

        int typeValue = parseStringValue(replyUrl, TAG_TYPE).toInt();
        int idValue = parseStringValue(replyUrl, TAG_ID).toInt();

        // Leemos el retorno
        QByteArray jsonBA(networkReply->readAll());
        QString str = QString(jsonBA);

        // Obtenemos los datos.
        bool succes = false;
        if(query.requestType == NEWS_SEARCH)
            succes = GetResultData(str, query);
        else
            succes = GetResultFromHtml(str, query);

        if(!succes)
        {
            //printf("Ha habido algún problema\n");
			if(FLAG_DEBUG)
			{
				query.status = CANT_RECOVERY_DATA;
				QFile outLog(QDir::currentPath()+"/requestLog.txt");
				outLog.open(QFile::WriteOnly | QFile::Append);
				QTextStream out(&outLog);
				out << QString("Time: [%1]").arg(QTime::currentTime().toString());
				QString ss = QString("Error con llamada: %1, tipo: %2 \n url: %3 \n no se ha posido extraer el valor.\n").arg(idValue).arg(typeValue).arg(replyUrl);
				out << QString("---- Thread %1 ----").arg(idThread) << endl << ss << QString("----------------------") << endl;
				out << QString("->>> El retorno es:") << endl << str << endl << QString("----------------------") << endl;
				outLog.close();
			}
        }


        if(FLAG_DEBUG)
        {
            QFile outLog(QDir::currentPath()+"/requestLog.txt");
            outLog.open(QFile::WriteOnly | QFile::Append);
            QTextStream out(&outLog);
            out << QString("Time: [%1]").arg(QTime::currentTime().toString());
            QString ss = QString("RECIBIDO!!!!: %1\n\n").arg(query.estimatedResultCount);
            out << QString("---- Thread %1 ----").arg(query.requestId) << endl << ss << QString("----------------------") << endl << endl;
            out << QString("---- Url: %1 ----").arg(replyUrl) << endl << ss << QString("----------------------") << endl << endl;
            out << QString("->>> El retorno es:") << endl << str << endl << QString("----------------------") << endl;
            outLog.close();
        }


    }
    else
    {
        errorNum = networkReply->error();
        if(FLAG_DEBUG)
        {
            QFile outLog(QDir::currentPath()+"/requestLog.txt");
            outLog.open(QFile::WriteOnly | QFile::Append);
            QTextStream out(&outLog);
            out << QString("Time: [%1]").arg(QTime::currentTime().toString());
            QString ss = QString("Error: %1\n\n").arg(errorNum);
            out << QString("---- Thread %1 ----").arg(query.requestId) << endl << ss << QString("----------------------") << endl << endl;

            if(errorNum != QNetworkReply::TimeoutError)
            {
                QByteArray jsonBA(networkReply->readAll());
                QString str = QString(jsonBA);
                out << QString("---- Respuesta:\n\n %1 ----\n\n\n").arg(str);

            }
            else
            {
                out << QString("Vamos a esperar un poco porque hay alg??n problema de bloqueo.");
                emit esperarUnPoco();


            }
            outLog.close();
        }
    }

    status = TH_FINISHED;
    emit echoSearch();
}

bool searchThread::GetResultFromHtml(QString str, requestData& data)
{
    //FILE* fout;
    //fout = fopen("temp_requests.txt", "a");
    QString text = str;

    if(text.size() == 0)
        return false;

    QString texto;
    long long resultados;
    int position;

    //fprintf(fout, "\nPeticion: %s\n\n\n",data.request.toStdString().c_str()); fprintf(fout,"\n");fflush(fout);
    position = text.indexOf(prefix01);
    if(position == -1 )
    {
        QString temp = prefix01;
        prefix01 = prefix02; // En este caso hacemos el cambio de prefijo.
        position = text.indexOf(prefix01);
        if(position == -1 )
        {
            data.status = -1;
            data.estimatedResultCount = -1;
            //fprintf(fout, "NO HA FUNCIONADO\n Text: %s \n con sufijo1: %s y sufijo2:%s\n",texto.toStdString().c_str(), temp.toStdString().c_str(), prefix02.toStdString().c_str()); printf("\n");fflush(0);
            //fprintf(fout, "\n\n\nText: \n%s\n\n\n\n",text.toStdString().c_str()); printf("\n");fflush(fout);
            //fprintf(fout, "\n\n\nError: \n%s\n\n\n\n",data.errorString); printf("\n");fflush(fout);

            //fclose(fout);
            return false;
        }
    }


    texto = text.right(text.size()-position-prefix01.length());//.left(40+sufix01.length());
    //position = texto.indexOf(sufix01);
    position = texto.indexOf("<");
    texto = texto.left(position);

    //printf("Text init: %s con sufijo: %s",texto.toStdString().c_str(), sufix01.toStdString().c_str()); printf("\n");fflush(0);
    // Ahora eliminamos las letras dem‡s cosas que estorban
    texto.remove(QRegExp("[A-Z]"));
    texto.remove(QRegExp("[a-z]"));
    texto.remove(',');
    texto.remove('.');

    //fprintf(fout, "Solo numero: %s",texto.toStdString().c_str()); printf("\n");fflush(fout);

    resultados = texto.toLongLong();

    data.status = 200;
    data.estimatedResultCount = resultados;

    //fclose(fout);

    return true;
}

bool searchThread::GetResultData(QString str, requestData& data)
{

        QString searchString = "responseStatus\":";
        int iniValue = str.lastIndexOf(searchString) + searchString.length();
        //QRegExp exp("}");
        int lastId = str.indexOf("}",iniValue);

        // Lectura del responseStatus entre ""
        QString sNodeValue;
        for(int i = iniValue; i < lastId; i++)
            sNodeValue.push_back(str[i]);

        data.status = sNodeValue.toInt();

        // En funci??n de este valor leemos m?¡s cosas
        if(data.status != 200)
        {
            if(data.status != 403)
            {
                // Ha habido alg??n problema
                searchString = "responseDetails\":";
                int iniValue = str.lastIndexOf(searchString) + searchString.length()+1;
                int lastId = str.indexOf(",",iniValue)-1;

                //printf("Ha habido algún problema\n");
                // Lectura del responseStatus entre ""
                sNodeValue.clear();
                for(int i = iniValue; i < lastId; i++)
                    sNodeValue.push_back(str[i]);

               // printf("Error:%s", data.errorString.toStdString().c_str());
                //printf("%d - %s\n ", lastId-iniValue, sNodeValue.toStdString().c_str());
                //printf("Status: %d\n", data.status);

                data.errorString = sNodeValue;
            }
        }
        else
        {
            // No ha habido problemas
            searchString = "estimatedResultCount\":";
            int iniValue = str.lastIndexOf(searchString) + searchString.length()+1;
            int lastId = str.indexOf(",",iniValue)-1;

            // Lectura del responseStatus entre ""
            sNodeValue.clear();
            for(int i = iniValue; i < lastId; i++)
                sNodeValue.push_back(str[i]);

            data.estimatedResultCount = sNodeValue.toLongLong();

        }

        return true;
}

int searchThread::doLoginSearch(QString PrevSearch)
{
    QUrl serviceUrl = QUrl(PrevSearch);
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrl postData;

	QNetworkReply* networkReply = networkManager.post(request,postData.toEncoded(QUrl::FullyEncoded));
    //QNetworkReply* networkReply = networkManager.get(QNetworkRequest(QUrl(PrevSearch)));

    QEventLoop loop;
    QObject::connect(networkReply, SIGNAL(readyRead()), &loop, SLOT(quit()));

    // Execute the event loop here, now we will wait here until readyRead() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    if (!networkReply->error())
    {
        QByteArray jsonBA(networkReply->readAll());
        QString str = QString(jsonBA);

        //printf("Text init: %s",str.toStdString().c_str()); printf("\n");fflush(0);

        int position = str.indexOf("</constantinopla>");
        str = str.left(position);
        str = str.right(10);

        // Ahora eliminamos las letras demas cosas que estorban
        str.remove(QRegExp("[A-Z]"));

        //printf("Solo el valor: %s",str.toStdString().c_str()); printf("\n");fflush(0);

        int idLog = str.toInt();

        return idLog;
    }
    return -1;
}

QString replaceSpecialChars(QString url)
{
    url = url.replace("+","%20");
    url = url.replace("‡","%C3%A1"); //a
    url = url.replace("Ž","%C3%A9"); //e
    url = url.replace("’","%C3%AD"); //i
    url = url.replace("—","%C3%B3"); //o
    return url.replace("œ","%C3%BA"); //u
}

void getDateValues(QString& str, int& day, int& month, int& year)
{
	day = 0;
	month = 0;
	year = 0;

	if(str.size() > 8)
		return;

	day = str.left(2).toInt();
	month = str.mid(3,2).toInt();
	year = str.right(2).toInt()+2000;
}

void searchThread::doSearch()
{
    QString url;
    status = TH_RUNNING;

    // 1. Evaluamos que parametros son necesarios
    QString pars = "";
    QString reqInfo = QString("%1=%2&%3=%4").arg(TAG_TYPE).arg(query.requestType).arg(TAG_ID).arg(query.requestId);

    QUrlQuery UrlEncoded;
    QString temp;

    // Traduccion general de algunos terminos del url

    // A. signos de sumar... para que lo coja como espacios -> quiz‡s no es lo mejor
    //url = url.replace("+","%20");

    // B. los acentos.
    //url = url.replace("‡","%C3%A1"); //a
    //url = url.replace("Ž","%C3%A9"); //e
    //url = url.replace("’","%C3%AD"); //i
    //url = url.replace("—","%C3%B3"); //o
    //url = url.replace("œ","%C3%BA"); //u

    QString urlString;
	QString encUrl;
    int stop;

    // 3. Construye la direccion de busqueda.
    switch(query.requestType)
    {
    case WEB_SEARCH:
		if(query.useDateConstraint)
		{
			printf("Fecha: %s - %s\n", query.ldate, query.hdate);

			int day,month,year;
			getDateValues(query.ldate,day,month,year);
			int startDate = getJulianDate(day,month,year);

			getDateValues(query.hdate,day,month,year);
			int endDate = getJulianDate(day,month,year);

			query.request.append(QString("+daterange:%1-%2").arg(startDate).arg(endDate));

			//url = url.append(QString("&%1=%2").arg(TAG_LDATE).arg((query.ldate).replace("/", "%2F")));
			//url = url.append(QString("&%1=%2").arg(TAG_HDATE).arg((query.hdate).replace("/", "%2F")));

			//temp = QString("cdr:1,cd_min:") + query.ldate + QString(",cd_max:") + query.hdate;
			//temp = temp.replace("/", "%2F");
			//temp = temp.replace(":", "%3A");
			//temp = temp.replace(",", "%2C");

			//url = url.append(QString("&%1=%2").arg(TAG_LDATE002).arg(temp));
		}

        if(!useVerbatim)
        {
            url = QString(GWEB_URL).arg(query.request);
        }
        else
        {
            url = QString(GWEB_URL_VERBATIM).arg(query.request);
        }

        if(query.useCountryConstraint)
            url.append(QString("&cr=%1").arg(query.country));

		//UrlEncoded.setQuery(replaceSpecialChars(url));
		//UrlEncoded.setQuery(url);

        break;
    case NEWS_SEARCH:
        reqInfo.append("&");
        url = QString(GNEWS_URL).arg(pars).arg(reqInfo).arg(query.request);
        //UrlEncoded.setQuery(replaceSpecialChars(url));
        break;
    case NEWS_SEARCH_HTML:
        url = QString(GNEWS_URL_HTML).arg(query.request);
        //UrlEncoded.setQuery(replaceSpecialChars(url));
        break;
    case YAHOO_WEB_SEARCH:
        url = QString(YWEB_URL).arg(query.request);
        //UrlEncoded.setQuery(replaceSpecialChars(url));
        break;
    case YAHOO_NEWS_SEARCH:
        url = QString(YNEWS_URL).arg(query.request);
        //UrlEncoded.setQuery(replaceSpecialChars(url));
        break;
    case NEWS_ARCHIVE_SEARCH:
        url = QString(GNEWS_ARCHIVE_URL).arg(query.request.replace(" ", "%20"));//.arg(datesLimit);
        //UrlEncoded.setQuery(replaceSpecialChars(url));

        temp = QString("cdr:1,cd_min:") + query.ldate + QString(",cd_max:") + query.hdate;
        temp = temp.replace("/", "%2F");
        temp = temp.replace(":", "%3A");
        temp = temp.replace(",", "%2C");

		url = url.append(QString("&%1=%2").arg(TAG_LDATE).arg((query.ldate).replace("/", "%2F")));
		url = url.append(QString("&%1=%2").arg(TAG_HDATE).arg((query.hdate).replace("/", "%2F")));
		url = url.append(QString("&%1=%2").arg(TAG_LDATE).arg(query.ldate));
		url = url.append(QString("&%1=%2").arg(TAG_HDATE).arg(query.hdate));
		url = url.append(QString("&%1=%2").arg(TAG_LDATE002).arg(temp));

		url = url.append(QString("&%1=%2").arg("pz").arg("1"));
		url = url.append(QString("&%1=%2").arg("cf").arg("all"));
		url = url.append(QString("&%1=%2").arg("ned").arg("es"));
		url = url.append(QString("&%1=%2").arg("gl").arg("es"));
		url = url.append(QString("&%1=%2").arg("as_occt").arg("any"));
		url = url.append(QString("&%1=%2").arg("as_drrb").arg("b"));

		url = url.append(QString("&%1=%2").arg("authuser").arg("0"));
		url = url.append(QString("&%1=%2").arg("safe").arg("strict"));

		url = url.replace("\"", "%22");

		/*
		UrlEncoded.addQueryItem(TAG_LDATE, (query.ldate).replace("/", "%2F"));
        UrlEncoded.addQueryItem(TAG_HDATE, (query.hdate).replace("/", "%2F"));
        UrlEncoded.addQueryItem(TAG_LDATE002, temp);

		UrlEncoded.addQueryItem("pz","1");
        UrlEncoded.addQueryItem("cf","all");
        UrlEncoded.addQueryItem("ned","es");
        UrlEncoded.addQueryItem("gl","es");
        UrlEncoded.addQueryItem("as_occt","any");
        UrlEncoded.addQueryItem("as_drrb","b");

        UrlEncoded.addQueryItem("authuser", "0");
        UrlEncoded.addQueryItem("safe","strict");
		*/

		 encUrl = replaceSpecialChars(url);

        break;
    default:
        url = QString(GWEB_URL).arg(pars).arg(reqInfo).arg(query.request);
        //UrlEncoded = QUrlQuery(url);
        break;
    }

    //UrlEncoded.addQueryItem(TAG_TYPE, QString(query.requestType));
    //UrlEncoded.addQueryItem(TAG_ID, QString(query.requestId));

    
    if(DEBUG_SEARCHES)
    {
		FILE *out;
        out = fopen(QString(QDir::currentPath() + "/searchVerbose.txt").toStdString().c_str(), "a");
        fprintf(out, "%s\n", UrlEncoded.toString().toStdString().c_str());
        fflush(out);
		fclose(out);
	}


    if(FLAG_DEBUG)
    {
        QFile outLog(QDir::currentPath()+"/requestLog.txt");
        outLog.open(QFile::WriteOnly | QFile::Append);
        QTextStream out(&outLog);
        out << QString("Time: [%1]").arg(QTime::currentTime().toString());
        QString ss = QString("Url: %1\n\n").arg(url);
        out << QString("---- Thread %1 ----").arg(query.requestId) << endl << ss << QString("----------------------") << endl << endl;
        outLog.close();
    }

    //printf(url.toStdString().c_str()); printf("\n"); fflush(0);
    //printf(UrlEncoded.toString().toStdString().c_str()); printf("\n"); fflush(0);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=ISO-8859-1");
    //request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/535.7 (KHTML, like Gecko) Chrome/16.0.912.77 Safari/535.7");
	request.setRawHeader("User-Agent", "Chrome/16.0.912.77");
	request.setAttribute(QNetworkRequest::ConnectionEncryptedAttribute, true);
	QUrl urlQuery;
	urlQuery.setQuery(UrlEncoded);
	request.setUrl(urlQuery);

	//Solo busquedas con google --- ¿porque?
	QString newUrlForGoogle = url.append(QString("&%1=%2").arg(TAG_TYPE).arg(query.requestType));
	newUrlForGoogle = newUrlForGoogle.append(QString("&%1=%2").arg(TAG_ID).arg(query.requestId));
	networkManager.get(QNetworkRequest(QUrl(newUrlForGoogle)));

	//printf(url.toStdString().c_str()); printf("\n"); fflush(0);
    
	//networkManager.get(QNetworkRequest(QUrl("https://www.google.es/search?site=&source=hp&q=nadal+daterange%3A2456546-2456556&oq=nadal+daterange%3A2456546-2456556&gs_l=hp.3...868.868.0.1681.1.1.0.0.0.0.141.141.0j1.1.0....0...1c.1.29.hp..1.0.0.nem_Hgz6Tao")));
}

