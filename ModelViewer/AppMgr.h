#ifndef APPMGR_H
#define APPMGR_H

#include <QtCore/qthread.h>
#include <QtCore/qobject.h>

#include <DataStructures\DataStructures.h>
#include <DataStructures\scene.h>
#include <Computation\ComputationMgr.h>

// This class manages the process of computing
class AppMgr : public QThread
{ 
	Q_OBJECT

public:
    AppMgr(QObject *parent = 0);
    ~AppMgr();

	void setComputeMgr(ComputationMgr* _mgr)
	{
		mgr = _mgr;
	}

protected:
    void run();
	ComputationMgr* mgr;

public slots:
    void stopProcess();

private:
    bool m_abort;
    QMutex mutex;

signals:
    void resultReady(const QString &s);
};

/*
void MyObject::startWorkInAThread();
{
    WorkerThread *workerThread = new WorkerThread(this);
    connect(workerThread, &WorkerThread::resultReady, this, &MyObject::handleResults);
    connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
}
*/

#endif // MAINWINDOW_H