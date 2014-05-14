#ifndef QUERYREADER_H
#define QUERYREADER_H

#define FLAG_DEBUG 1

#include <QString>
#include <QStringList>

int readFile(QStringList& listOfQueryNames, QString& fileName);
int readFile_for_archive(QStringList& listOfQueryNames, QStringList& listOfQueries, QStringList& dates, QString& fileName);
int readDatesFile(QVector<QStringList>& dates, QString& fileName);

#endif // QUERYREADER_H
