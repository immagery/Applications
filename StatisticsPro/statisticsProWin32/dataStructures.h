#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <QObject>
#include <QTime>
#include <QStringList>
#include <QVector>
#include <QString>

#define FLAG_DEBUG 1

#define LOGIN_URL "http://www.immagery.com/statisticsPro/ldConstantinopolis.html"

#define WEB_SEARCH 0
#define NEWS_SEARCH 1
#define NEWS_SEARCH_HTML 2
#define YAHOO_WEB_SEARCH 3
#define YAHOO_NEWS_SEARCH 4
#define NEWS_ARCHIVE_SEARCH 5


#define SCHEMES_COUNT 6

#define GWEB_URL          "http://www.google.com/search?q=%1&aql=&oq=?v=1.0"
#define GWEB_URL_VERBATIM "http://www.google.com/search?q=%1&aql=&oq=?v=1.0&tbs=li:1"
#define GNEWS_URL "http://ajax.googleapis.com/ajax/services/search/news?v=1.0%1&%2q=%3"
#define GNEWS_URL_HTML "http://www.google.com/search?tbm=nws&btnmeta_news_search=1&q=%1&oq=%1&opto=on&safe=strict"

// ANTIGUO 29/ENE/2012: #define GNEWS_ARCHIVE_URL "http://www.google.com/search?tbm=nws&hl=es&gl=es&as_q=%1&as_scoring=r&btnG=Buscar&as_qdr=a%2&as_drrb=b&as_nsrc=&as_nloc=&as_author=&as_occt=any&"

//#define GNEWS_ARCHIVE_URL "https://www.google.com/search?tbm=nws&btnmeta_news_search=1&hl=es&gl=es&q=%1&as_scoring=r&btnG=Buscar&as_qdr=a%2&as_drrb=b&as_nsrc=&as_nloc=&as_author=&as_occt=any&"

//#define GNEWS_ARCHIVE_URL"http://www.google.com/search?pz=1&tbm=nws&ned=es&cf=all&hl=es&gl=es&as_q=%1&as_qdr=a%2&as_drrb=b&as_occt=any&authuser=0&"

//#define GNEWS_ARCHIVE_URL "http://www.google.com/search?pz=1&tbm=nws&cf=all&btnmeta_news_search=1&hl=es&gl=es&as_q=%1&as_scoring=r&btnG=Buscar&as_qdr=a%2&as_drrb=b&as_nsrc=&as_nloc=&as_author=&as_occt=any&authuser=0&"

#define GNEWS_ARCHIVE_URL "http://www.google.com/search?tbm=nws&as_q=%1"

#define SAVE_STRICT "&safe=active"
#define YWEB_URL "http://es.search.yahoo.com/search?p=%1&fr=yfp-t-705&vm=r&rd=r1"
#define YNEWS_URL "http://es.news.search.yahoo.com/search/news?vc=&p=%1&toggle=1&cop=mss&ei=UTF-8&fr=yfp-t-705"

#define TAG_TYPE "spro_Type"
#define TAG_ID "spro_Id"
#define TAG_LDATE "as_mindate"
#define TAG_HDATE "as_maxdate"

#define TAG_LDATE002 "tbs"
#define TAG_JOIN "%2F"
#define TAG_HDATE002 ""

//GOOGLE NEWS
#define PREFIX01_GNEWS "<div id=\"resultStats\">"
#define PREFIX02_GNEWS "</b> de <b>"
#define SUFIX01_GNEWS "</div>"
#define SUFIX02_GNEWS "</div>"

//GOOGLE ARCHIVE
#define PREFIX01_GNEWS_ARCH "<div id=\"resultStats\">"
#define PREFIX02_GNEWS_ARCH "</b> de <b>"
#define SUFIX01_GNEWS_ARCH "</div>"
#define SUFIX02_GNEWS_ARCH "</div>"

//YAHOO NEWS
#define PREFIX01_YNEWS "id=\"resultCount\" class=\"count\">"
#define PREFIX02_YNEWS "otro prefijo"
#define SUFIX01_YNEWS  "</span>"
#define SUFIX02_YNEWS "</span>"

//YAHOO_WEB
#define PREFIX01_YWEB "id=\"resultCount\">"
#define PREFIX02_YWEB "otro prefijo"
#define SUFIX01_YWEB  "</span>"
#define SUFIX02_YWEB  "</span>"

//GOOGLE_WEB
#define PREFIX01_GWEB "<div id=\"resultStats\">"
#define PREFIX02_GWEB "<div>Aproximadamente"
#define SUFIX01_GWEB  "result"
#define SUFIX02_GWEB  "</span>"

/*
#define TAG_LDATE002 "tbs=cdr%3A1%2Ccd_min%3A"
#define TAG_JOIN "%2F"
#define TAG_HDATE002 "%2Ccd_max%3A"
*/

// %3A -> :
// %2C -> ,
// %2F -> /

//tbs=cdr:1,cd_min:20/08/11,cd_max:19/09/11 &

#define NODATA 200
#define CANT_RECOVERY_DATA 999

#define DEFAULT_QUERY_BLOCK 200

#define INTER_BlOCK_WAIT_TIME 300  /*in seconds*/
#define WAIT_TIME 3

#define NUM_OF_ENVIRONMENTS 6

#define USE_LOCAL_PROXY 0

// PROXY_POR_DEFECTO
#define default_proxy_name "192.168.1.3"
#define default_proxy_port 3128


class requestData
{
public:

    QString request;
    QString requestName;
    QString errorString;

    int status;
    long long estimatedResultCount;
    int requestId;
    int requestType;
    int waitLapsusCounter;

    QString ldate, hdate, country;

    bool useCountryConstraint;
    bool useDateConstraint;

    int row;
    int col;

    public:
    requestData()
    {
        request = "";
        requestName = "";
        errorString = "";

        status = -1;
        requestId = -1;
        requestType = -1;
        estimatedResultCount = -1;
        waitLapsusCounter = -1;

        ldate = "";
        hdate = "";
        useDateConstraint = 0;

        row = -1;
        col = -1;

        country = "";
        useCountryConstraint = 0;

    }

    requestData(QString _request, int id, int type)
    {
        //request = _request.replace(" ","%20");
        request = _request.replace(" ","+");
        requestName = _request;
        errorString = "";

        requestId = id;
        requestType = type;

        status = -1;
        requestId = -1;
        requestType = -1;
        estimatedResultCount = -1;
        waitLapsusCounter = -1;

        ldate = "";
        hdate = "";
        useDateConstraint = 0;

        row = -1;
        col = -1;

        country = "";
        useCountryConstraint = 0;
    }

    requestData(const requestData& rd)
    {
        request = rd.request;
        requestName = rd.requestName;
        errorString = rd.errorString;
        requestId = rd.requestId;
        requestType = rd.requestType;
        status = rd.status;
        requestId = rd.requestId;
        requestType = rd.requestType;
        estimatedResultCount = rd.estimatedResultCount;
        waitLapsusCounter = rd.waitLapsusCounter;
        ldate = rd.ldate;
        hdate = rd.hdate;
        row = rd.row;
        col = rd.col;

        country = rd.country;
        useCountryConstraint = rd.useCountryConstraint;
        useDateConstraint = rd.useDateConstraint;

    }

    void operator=( const requestData& rd)
    {
        request = rd.request;
        requestName = rd.requestName;
        errorString = rd.errorString;
        requestId = rd.requestId;
        requestType = rd.requestType;
        status = rd.status;
        requestId = rd.requestId;
        requestType = rd.requestType;
        estimatedResultCount = rd.estimatedResultCount;
        waitLapsusCounter = rd.waitLapsusCounter;
        ldate = rd.ldate;
        hdate = rd.hdate;
        row = rd.row;
        col = rd.col;
        country = rd.country;
        useCountryConstraint = rd.useCountryConstraint;
        useDateConstraint = rd.useDateConstraint;
    }

    void setData(QString _request, int id, int type)
    {
        requestName = _request;
        //request = _request.replace(" ","%20");
        request = _request.replace(" ","+");
        requestId = id;
        requestType = type;
        status = -1;
        estimatedResultCount = -1;
        errorString = "";
    }
};

#endif // DATASTRUCTURES_H
