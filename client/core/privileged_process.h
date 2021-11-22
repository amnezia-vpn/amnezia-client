#ifndef PRIVILEGED_PROCESS_H
#define PRIVILEGED_PROCESS_H

#include <QObject>

#ifndef Q_OS_IOS
#include "rep_ipc_process_interface_replica.h"
// This class is dangerous - instance of this class casted from base class,
// so it support only functions
// Do not add any members into it
//
class PrivilegedProcess : public IpcProcessInterfaceReplica
{
    Q_OBJECT
public:
    PrivilegedProcess();
    ~PrivilegedProcess() override;

    void waitForFinished(int msecs);

};

#endif // PRIVILEGED_PROCESS_H

#else // defined Q_OS_IOS
class IpcProcessInterfaceReplica {};
class PrivilegedProcess {};
#endif


