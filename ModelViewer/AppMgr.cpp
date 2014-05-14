#include "AppMgr.h"


AppMgr::AppMgr(QObject *parent)
    : QThread(parent)
{
    m_abort = false;
}

AppMgr::~AppMgr()
{
    mutex.lock();
    m_abort = true;
    mutex.unlock();

    wait();
}

void AppMgr::stopProcess()
{
    mutex.lock();
    m_abort = true;
    mutex.unlock();
}

void AppMgr::run() {
    
	mgr->updateAllComputations();

    /* expensive or blocking operation  */
    //emit resultReady(result);
}