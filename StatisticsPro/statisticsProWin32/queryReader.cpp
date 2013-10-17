#include <QFile>
#include <QTextStream>
#include <QVector>

#include <iostream>
#include <fstream>

#include "queryReader.h"

// Return: num of Queries
int readFile_for_archive(QStringList& listOfQueryNames, QStringList& listOfQueries, QStringList& dates, QString& fileName)
{
    // Fichero de b√∫squedas
    QFile inFile(fileName);
    if(!inFile.exists())
        return -1;
    inFile.open(QFile::ReadOnly);
    QTextStream in(&inFile);

    // Fichero de restricciones
    QFile inFileRestrictions(fileName.left(fileName.size()-3).append("rst"));
    if(!inFileRestrictions.exists())
        return -1;
    inFileRestrictions.open(QFile::ReadOnly);
    QTextStream inRestrictions(&inFileRestrictions);

    QString restrictions = inRestrictions.readLine().trimmed();
    dates = restrictions.split("#");

    int queryCounter = 0;
    while(!in.atEnd())
    {
        // Leemos l√≠nea a l√≠nea el contenido y lo parseamos para que se pueda buscar directamente.
        QString query = in.readLine().trimmed();

        //printf("Busqueda:%s\n", query.toStdString().c_str()); fflush(0);

        // En el caso de que haya una l√≠nea en blanco la saltamos.
        if(query.isEmpty())
            continue;

        listOfQueryNames.push_back(query);
        //listOfQueries.push_back(query.replace(" ","%20"));
        //listOfQueries.push_back(query.replace(" ","+"));

        queryCounter++;
    }

    return queryCounter;
}

int readDatesFile(QVector<QStringList>& dates, QString& fileName)
{
    dates.clear();

    // Fichero de restricciones
    QFile inFileRestrictions(fileName.left(fileName.size()-3).append("rst"));
    if(!inFileRestrictions.exists())
        return -1;

    inFileRestrictions.open(QFile::ReadOnly);
    QTextStream inRestrictions(&inFileRestrictions);

    while(!inRestrictions.atEnd())
    {
        QString restrictions = inRestrictions.readLine().trimmed();
        if(restrictions.isEmpty())
            continue;

        dates.push_back(restrictions.split("#"));
    }

    return dates.size();
}

int readFile(QStringList& listOfQueryNames, QString& fileName)
{
    std::ifstream fin;
    fin.open(fileName.toStdString().c_str(), std::ifstream::in);

    //if(!fin) return -1;

    //QFile inFile(fileName);
    //if(!inFile.exists())
    //    return -1;

    //inFile.open(QFile::ReadOnly);

    //QTextStream in(&inFile);
    //in.setCodec("utf-8");

    int queryCounter = 0;
    //while(!in.atEnd())
    while(fin)
    {
        std::string line;
        std::getline(fin, line);

        // Leemos l√≠nea a l√≠nea el contenido y lo parseamos para que se pueda buscar directamente.
        //QString query = in.readLine().trimmed();

        //QString sdd = "hÛl·l·";
        if(FLAG_DEBUG)
        {
            //printf("Busqueda:%s hÛl‡\n", query.toStdString().c_str());
            printf("Busqueda:%s\n", line.c_str());
            fflush(0);
        }

        // En el caso de que haya una l√≠nea en blanco la saltamos.
        if(line.empty())
        //if(query.isEmpty())
            continue;

        listOfQueryNames.push_back(QString(line.c_str()));
        queryCounter++;
    }

    fin.close();

    return queryCounter;
}
