#include "ipcserver.h"

#include <QDateTime>

IpcServer::IpcServer(QObject *parent):
    IpcInterfaceSource(parent)
{}

int IpcServer::createPrivilegedProcess()
{
    m_localpid++;

    ProcessDescriptor pd;
    pd.serverNode->setHostUrl(QUrl(amnezia::getIpcProcessUrl(m_localpid)));
    pd.serverNode->enableRemoting(pd.ipcProcess.data());

    m_processes.insert(m_localpid, pd);

    return m_localpid;
}
